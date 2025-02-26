/**
 * @file http_worker.c
 * @brief HTTP worker
 *
 * This file contains code that runs in omni_httpd http workers that serve requests. These
 * workers are started by the master worker.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <h2o.h>
#include <h2o/http1.h>
#include <h2o/http2.h>
#include <h2o/websocket.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <access/xact.h>
#include <catalog/namespace.h>
#include <catalog/pg_authid.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <postmaster/bgworker.h>
#include <storage/latch.h>
#include <storage/lmgr.h>
#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/inet.h>
#include <utils/lsyscache.h>
#include <utils/snapmgr.h>
#include <utils/syscache.h>
#include <utils/timestamp.h>
#if PG_MAJORVERSION_NUM >= 13
#include <postmaster/interrupt.h>
#endif
#include <access/heapam.h>
#include <access/table.h>
#include <catalog/pg_proc.h>
#include <common/hashfn.h>
#include <parser/parse_func.h>
#include <storage/ipc.h>
#include <tcop/pquery.h>
#include <utils/datum.h>
#include <utils/pidfile.h>
#include <utils/typcache.h>
#include <utils/uuid.h>

#include <metalang99.h>

#include <libpgaug.h>
#include <omni_sql.h>
#include <sum_type.h>

#include "event_loop.h"
#include "fd.h"
#include "http_worker.h"
#include "omni_httpd.h"
#include "urlpattern.h"

#if H2O_USE_LIBUV == 1
#error "only evloop is supported, ensure H2O_USE_LIBUV is not set to 1"
#endif

#include "http_status_reasons.h"

h2o_multithread_receiver_t handler_receiver;
h2o_multithread_queue_t *handler_queue;

static clist_listener_contexts listener_contexts = {NULL};

static Oid handler_oid = InvalidOid;
static Oid websocket_handler_oid = InvalidOid;
static Oid websocket_on_open_oid = InvalidOid;
static Oid websocket_on_close_oid = InvalidOid;
static Oid websocket_on_message_text_oid = InvalidOid;
static Oid websocket_on_message_binary_oid = InvalidOid;

static TupleDesc request_tupdesc;
enum http_method {
  http_method_GET,
  http_method_HEAD,
  http_method_POST,
  http_method_PUT,
  http_method_DELETE,
  http_method_CONNECT,
  http_method_OPTIONS,
  http_method_TRACE,
  http_method_PATCH,
  http_method_last
};
static Oid http_method_oids[http_method_last] = {InvalidOid};
static char *http_method_names[http_method_last] = {"GET",     "HEAD",    "POST",  "PUT",  "DELETE",
                                                    "CONNECT", "OPTIONS", "TRACE", "PATCH"};

typedef struct {
  Oid routeroid;
  uint32 hash;
  SPIPlanPtr plan;
  uint32 status;
} RouterQueryHashEntry;

struct routerqueryhash_hash;
static struct routerqueryhash_hash *routerqueryhash = NULL;

#define SH_PREFIX routerqueryhash
#define SH_ELEMENT_TYPE RouterQueryHashEntry
#define SH_KEY_TYPE Oid
#define SH_KEY routeroid
#define SH_HASH_KEY(tb, key) key
#define SH_EQUAL(tb, a, b) a == b
#define SH_SCOPE static
#define SH_STORE_HASH
#define SH_GET_HASH(tb, a) a->hash
#define SH_DECLARE
#define SH_DEFINE
#include <lib/simplehash.h>

/**
 * Portal that we re-use in the handler
 *
 * It's kind of a "fake portal" in a sense that it only exists so that the check in
 * `ForgetPortalSnapshots()` doesn't fail with this upon commit/rollback in SPI:
 *
 * ```
 * portal snapshots (0) did not account for all active snapshots (1)
 * ```
 *
 * This is happening because we do have an anticipated snapshot when we call in non-atomic
 * fashion. In case when a languages is capable of handling no snapshot, that's all good –
 * PL/pgSQL, for example, works. However, if we call, say, an SQL function, it does not expect
 * to be non-atomic and hence needs a snapshot to operate in.
 */
static Portal execution_portal;
/**
 * Call context for all handler calls
 */
static CallContext *non_atomic_call_context;

/**
 * Handler context
 */
static MemoryContext HandlerContext;

static void on_message(h2o_multithread_receiver_t *receiver, h2o_linklist_t *messages) {
  while (!h2o_linklist_is_empty(messages)) {
    h2o_multithread_message_t *message =
        H2O_STRUCT_FROM_MEMBER(h2o_multithread_message_t, link, messages->next);

    handler_message_t *msg = (handler_message_t *)messages->next;
    h2o_linklist_unlink(&message->link);

    switch (msg->type) {
    case handler_message_http: {
      request_message_t *request_msg = &msg->payload.http.msg;

      pthread_mutex_t *mutex = &request_msg->mutex;
      pthread_mutex_lock(mutex);
      handler(msg);
      pthread_mutex_unlock(mutex);
      break;
    }
    default:
      handler(msg);
      switch (msg->type) {
      case handler_message_websocket_open:
        break;
      case handler_message_websocket_message:
        free(msg->payload.websocket_message.message);
        break;
      case handler_message_websocket_close:
        break;
      default:
        Assert(false); // shouldn't be here
      }
      free(msg);
      break;
    }
  }
}

static void sigusr2() {
  atomic_store(&worker_reload, true);
  h2o_multithread_send_message(&event_loop_receiver, NULL);
  h2o_multithread_send_message(&handler_receiver, NULL);
}

static int exit_code = 0;

SPIPlanPtr router_query;

static void sigterm() {
  atomic_store(&worker_running, false);
  // master_worker sets semaphore to INT32_MAX to signal normal termination
  // NULL semaphore means we're not being started anymore
  exit_code =
      (!BackendInitialized || semaphore == NULL || pg_atomic_read_u32(semaphore) == INT32_MAX) ? 0
                                                                                               : 1;
  h2o_multithread_send_message(&event_loop_receiver, NULL);
  h2o_multithread_send_message(&handler_receiver, NULL);
}

/**
 * HTTP worker entry point
 *
 * This is where everything starts. The worker is responsible for accepting connections (when
 * applicable) and handling requests. The worker handles requests for a single database.
 * @param db_oid Database OID
 */
void http_worker(Datum db_oid) {
  atomic_store(&worker_running, true);
  atomic_store(&worker_reload, true);

  // We call this before we unblock the signals as necessitated by the implementation
  setup_server();

  // Block signals except for SIGUSR2 and SIGTERM
  pqsignal(SIGUSR2, sigusr2); // used to reload configuration
  pqsignal(SIGTERM, sigterm); // used to terminate the worker

  // Start thread that will be servicing `worker_event_loop` and handling all
  // communication with the outside world. Current thread will be responsible for
  // configuration reloads and calling handlers.
  pthread_t event_loop_thread;
  event_loop_suspended = true;

  // This MUST happen before starting event_loop
  // AND before unblocking signals as signals use this receiver
  event_loop_register_receiver();
  pthread_create(&event_loop_thread, NULL, event_loop, NULL);

  // Connect worker to the database
  BackgroundWorkerInitializeConnectionByOid(db_oid, InvalidOid, 0);
  BackgroundWorkerUnblockSignals();

  if (MyBgworkerEntry->bgw_notify_pid == 0) {
    // We are being restarted when somebody crashed. We would otherwise have master's PID here
    return;
  }

  if (!BackendInitialized) {
    // omni_httpd is being shut down
    return;
  }

  if (semaphore == NULL) {
    // omni_httpd is being shut down
    return;
  }

  // Initialize the router query cache
  routerqueryhash = routerqueryhash_create(TopMemoryContext, 8192, NULL);

  // Get necessary OIDs and tuple descriptors
  PG_TRY();
  {
    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    PushActiveSnapshot(GetTransactionSnapshot());

    // Cache these. These calls require a transaction, so we don't want to do this on demand in
    // non-atomic executions in the handler
    http_request_oid();
    http_header_oid();
    http_response_oid();
    http_outcome_oid();
    urlpattern_oid();
    route_priority_oid();

    {
      // omni_httpd.websocket_handler(int,uuid,http_request)
      List *websocket_handler_func =
          list_make2(makeString("omni_httpd"), makeString("websocket_handler"));
      websocket_handler_oid = LookupFuncName(websocket_handler_func, 3,
                                             (Oid[3]){INT4OID, UUIDOID, http_request_oid()}, false);
      list_free(websocket_handler_func);
    }

    {
      // omni_httpd.websocket_on_open(uuid)
      List *websocket_on_open_func =
          list_make2(makeString("omni_httpd"), makeString("websocket_on_open"));
      websocket_on_open_oid = LookupFuncName(websocket_on_open_func, 1, (Oid[1]){UUIDOID}, false);
      list_free(websocket_on_open_func);
    }

    {
      // omni_httpd.websocket_on_close(uuid)
      List *websocket_on_close_func =
          list_make2(makeString("omni_httpd"), makeString("websocket_on_close"));
      websocket_on_close_oid = LookupFuncName(websocket_on_close_func, 1, (Oid[1]){UUIDOID}, false);
      list_free(websocket_on_close_func);
    }

    {
      // omni_httpd.websocket_on_message(uuid,text)
      List *websocket_on_message_text_func =
          list_make2(makeString("omni_httpd"), makeString("websocket_on_message"));
      websocket_on_message_text_oid =
          LookupFuncName(websocket_on_message_text_func, 2, (Oid[2]){UUIDOID, TEXTOID}, false);
      list_free(websocket_on_message_text_func);
    }

    {
      // omni_httpd.websocket_on_message(uuid,bytea)
      List *websocket_on_message_binary_func =
          list_make2(makeString("omni_httpd"), makeString("websocket_on_message"));
      websocket_on_message_binary_oid =
          LookupFuncName(websocket_on_message_binary_func, 2, (Oid[2]){UUIDOID, BYTEAOID}, false);
      list_free(websocket_on_message_binary_func);
    }

    // Save omni_httpd.http_request's tupdesc in TopMemoryContext
    // to persist it
    MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
    request_tupdesc = TypeGetTupleDesc(http_request_oid(), NULL);
    MemoryContextSwitchTo(old_context);

    // Populate HTTP method IDs
    for (int i = 0; i < http_method_last; i++) {
      http_method_oids[i] = DirectFunctionCall2(enum_in, PointerGetDatum(http_method_names[i]),
                                                ObjectIdGetDatum(http_method_oid()));
    }

    PopActiveSnapshot();
    AbortCurrentTransaction();
  }
  // This is important to catch because `StartBackgroundWorker` catches the error
  // and eventually goes to `proc_exit(1)` which cleans up shared memory and our
  // signal handlers try to access it.
  // This is not a very common path, it's primarily related to quick succession of
  // omni_httpd termination after startup.
  // Instead, we simply admit we can't initialize the worker, we concede to it and
  // gracefully exit.
  PG_CATCH();
  {
    ereport(WARNING, errmsg("omni_httpd is not ready, shutting omni_httpd down"));
    BackendInitialized = false;
    proc_exit(0);
  }
  PG_END_TRY();

  {
    // Prepare the persistent portal
    execution_portal = CreatePortal("omni_httpd", true, true);
    execution_portal->resowner = NULL;
    execution_portal->visible = false;
    PortalDefineQuery(execution_portal, NULL, "(no query)", CMDTAG_UNKNOWN, false, NULL
#if PG_MAJORVERSION_NUM >= 18
                      ,
                      NULL
#endif
    );
    PortalStart(execution_portal, NULL, 0, InvalidSnapshot);
  }

  {
    // Prepare query plans
    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    PushActiveSnapshot(GetTransactionSnapshot());

    SPI_connect();

    // Find tables of a certain type
    router_query = SPI_prepare("select router_relation, match_col_idx, handler_col_idx, "
                               "priority_col_idx from omni_httpd.available_routers",
                               0, (Oid[0]){});
    if (router_query == NULL) {
      ereport(ERROR, errmsg("can't prepare query"));
    }
    SPI_keepplan(router_query);

    SPI_finish();

    PopActiveSnapshot();
    AbortCurrentTransaction();
  }

  {
    // All call contexts are non-atomic
    MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
    non_atomic_call_context = makeNode(CallContext);
    non_atomic_call_context->atomic = false;
    MemoryContextSwitchTo(old_context);
  }

  HandlerContext =
      AllocSetContextCreate(TopMemoryContext, "omni_httpd handler context", ALLOCSET_DEFAULT_SIZES);

  while (atomic_load(&worker_running)) {
    bool worker_reload_test = true;
    if (atomic_compare_exchange_strong(&worker_reload, &worker_reload_test, false)) {

      SetCurrentStatementStartTimestamp();
      StartTransactionCommand();
      PushActiveSnapshot(GetTransactionSnapshot());

      SPI_connect();

      Oid nspoid = get_namespace_oid("omni_httpd", false);
      Oid listenersoid = get_relname_relid("listeners", nspoid);

      // The idea here is that we get handlers sorted by listener id the same way they were
      // sorted in the master worker (`by id asc`) and since they will arrive in the same order
      // in the list of fds, we can simply get them by the index.
      //
      // This table is locked by the master worker and thus the order of listeners will not change
      if (!ConditionalLockRelationOid(listenersoid, AccessShareLock)) {
        // If we can't lock it, something is blocking us (like `drop extension` waiting on
        // master worker to complete for its AccessExclusiveLock while master worker is holding
        // an ExclusiveLock waiting for http workers to signal their readiness)
        continue;
      }

      int handlers_query_rc = SPI_execute("select listeners.id "
                                          "from omni_httpd.listeners "
                                          "order by listeners.id asc",
                                          false, 0);

      UnlockRelationOid(listenersoid, AccessShareLock);

      // Allocate handler information in this context instead of current SPI's context
      // It will get deleted later.
      MemoryContext handlers_ctx =
          AllocSetContextCreate(TopMemoryContext, "handlers_ctx", ALLOCSET_DEFAULT_SIZES);
      MemoryContext old_ctx = MemoryContextSwitchTo(handlers_ctx);

      // This is where we record an ordered list of handlers
      List *handlers = NIL;
      // (which are described using this struct)
      struct pending_handler {
        int id;
      };

      if (handlers_query_rc == SPI_OK_SELECT) {
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        SPITupleTable *tuptable = SPI_tuptable;
        for (int i = 0; i < tuptable->numvals; i++) {
          HeapTuple tuple = tuptable->vals[i];
          bool id_is_null = false;
          Datum id = SPI_getbinval(tuple, tupdesc, 1, &id_is_null);

          struct pending_handler *handler = palloc(sizeof(*handler));
          handler->id = DatumGetInt32(id);
          handlers = lappend(handlers, handler);
        }

      } else {
        ereport(WARNING, errmsg("Error fetching configuration: %s",
                                SPI_result_code_string(handlers_query_rc)));
      }
      MemoryContextSwitchTo(old_ctx);

      // Roll back the transaction as we don't want to hold onto it anymore
      SPI_finish();
      PopActiveSnapshot();
      AbortCurrentTransaction();

      // Now that we have this information, we can let the master worker commit its update and
      // release the lock on the table.

      pg_atomic_add_fetch_u32(semaphore, 1);

      // When all HTTP workers will do the same, the master worker will start serving the
      // socket list.

      cvec_fd_fd fds = accept_fds(MyBgworkerEntry->bgw_extra);
      if (cvec_fd_fd_empty(&fds)) {
        continue;
      }

      // At first, assume all fds are new (so we need to set up listeners)
      cvec_fd_fd new_fds = cvec_fd_fd_clone(fds);
      // Disposing allocated listener/query pairs and their associated data
      // (such as sockets)
      {
        clist_listener_contexts_iter iter = clist_listener_contexts_begin(&listener_contexts);
        while (iter.ref != NULL) {
          // Do we have this socket in the new packet?
          c_foreach(it, cvec_fd_fd, new_fds) {
            // Compare by their master fd
            if (it.ref->master_fd == iter.ref->master_fd) {
              // If we do, continue using the listener
              clist_listener_contexts_next(&iter);
              // close the incoming  because we aren't going to be using it
              // and it may be polluting the fd table
              close(it.ref->fd);
              // and ensure we don't set up a listener for it
              cvec_fd_fd_erase_at(&new_fds, it);
              // process the next listener
              goto next_ctx;
            } else {
            }
          }
          // Otherwise, dispose of the listener

          if (iter.ref->socket != NULL) {
            h2o_socket_t *socket = iter.ref->socket;
            h2o_socket_export_t info;
            h2o_socket_export(socket, &info);
            h2o_socket_dispose_export(&info);
          }
          h2o_context_dispose(&iter.ref->context);
          MemoryContextDelete(iter.ref->memory_context);
          iter = clist_listener_contexts_erase_at(&listener_contexts, iter);
        next_ctx: {}
        }
      }

      // Set up new listeners as necessary
      c_FOREACH(i, cvec_fd_fd, new_fds) {
        int fd = (i.ref)->fd;

        listener_ctx c = {.fd = fd,
                          .master_fd = (i.ref)->master_fd,
                          .socket = NULL,
                          .memory_context =
                              AllocSetContextCreate(TopMemoryContext, "omni_httpd_listener_context",
                                                    ALLOCSET_DEFAULT_SIZES),
                          .accept_ctx = (h2o_accept_ctx_t){.hosts = config.hosts}};

        listener_ctx *lctx = clist_listener_contexts_push(&listener_contexts, c);

      try_create_listener:
        if (create_listener(fd, lctx) == 0) {
          h2o_context_init(&(lctx->context), worker_event_loop, &config);
          lctx->accept_ctx.ctx = &lctx->context;
        } else {
          if (errno == EINTR) {
            goto try_create_listener; // retry
          }
          int e = errno;
          ereport(WARNING, errmsg("socket error: %s", strerror(e)));
        }
      }
      cvec_fd_fd_drop(&new_fds);

      SetCurrentStatementStartTimestamp();
      StartTransactionCommand();
      PushActiveSnapshot(GetTransactionSnapshot());
      SPI_connect();

      // Now we're ready to work with the results of the query we made earlier:
      {
        // Here we have to track what was the last listener id
        int last_id = 0;
        // Here we have to track what was the last index
        int index = -1;

        ListCell *lc;
        foreach (lc, handlers) {
          struct pending_handler *handler = (struct pending_handler *)lfirst(lc);

          // Figure out socket index
          if (last_id != handler->id) {
            last_id = handler->id;
            index++;
          }

          const cvec_fd_fd_value *fd = cvec_fd_fd_at(&fds, index);
          Assert(fd != NULL);
          listener_ctx *listener_ctx = NULL;
          c_foreach(iter, clist_listener_contexts, listener_contexts) {
            if (iter.ref->master_fd == fd->master_fd) {
              listener_ctx = iter.ref;
              break;
            }
          }
          Assert(listener_ctx != NULL);

          // Set listener ID
          listener_ctx->listener_id = handler->id;
        }
        // Free everything in handlers context
        MemoryContextDelete(handlers_ctx);
        handlers = NIL;
      }

      cvec_fd_fd_drop(&fds);
      SPI_finish();
      PopActiveSnapshot();
      AbortCurrentTransaction();
    }

    event_loop_suspended = false;
    pthread_mutex_lock(&event_loop_mutex);
    pthread_cond_signal(&event_loop_resume_cond);
    pthread_mutex_unlock(&event_loop_mutex);

    bool running = atomic_load(&worker_running);
    bool reload = atomic_load(&worker_reload);

    // Handle requests until shutdown or reload is requested
    while ((running = atomic_load(&worker_running)) && !(reload = atomic_load(&worker_reload)) &&
           h2o_evloop_run(handler_event_loop, INT32_MAX))
      ;

    // Ensure the event loop is suspended while we're reloading
    if (reload || !running) {
      pthread_mutex_lock(&event_loop_mutex);
      while (event_loop_resumed) {
        pthread_cond_wait(&event_loop_resume_cond_ack, &event_loop_mutex);
      }
      pthread_mutex_unlock(&event_loop_mutex);
    }
  }

  clist_listener_contexts_drop(&listener_contexts);
  PortalDrop(execution_portal, false);
  // Exit with the designated exit code, unless we can see that postmaster is being shut down
  if (IsPostmasterBeingShutdown()) {
    exit_code = 0;
  }
  BackendInitialized = false;
  proc_exit(exit_code);
}

static inline int listener_ctx_cmp(const listener_ctx *l, const listener_ctx *r) {
  return (l->listener_id == r->listener_id && l->socket == r->socket && l->fd == r->fd) ? 0 : -1;
}

h2o_accept_ctx_t *listener_accept_ctx(h2o_socket_t *listener) {
  return &((listener_ctx *)listener->data)->accept_ctx;
}

/**
 * @brief Create a listening socket
 *
 * @param family
 * @param port
 * @param address
 * @return int
 */
int create_listening_socket(sa_family_t family, in_port_t port, char *address,
                            in_port_t *out_port) {
  struct sockaddr_in addr;
  struct sockaddr_in6 addr6;
  void *sockaddr;
  socklen_t socksize;
  int fd, reuseaddr_flag = 1;

  if (family == AF_INET) {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, address, &addr.sin_addr);
    addr.sin_port = htons(port);
    sockaddr = &addr;
    socksize = sizeof(addr);
  } else if (family == AF_INET6) {
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET;
    inet_pton(AF_INET6, address, &addr6.sin6_addr);
    addr6.sin6_port = htons(port);
    sockaddr = &addr6;
    socksize = sizeof(addr6);
  } else {
    return -1;
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ||
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_flag, sizeof(reuseaddr_flag)) != 0 ||
      bind(fd, (struct sockaddr *)sockaddr, socksize) != 0 || listen(fd, SOMAXCONN) != 0) {
    return -1;
  }

  if (out_port != NULL) {
    if (getsockname(fd, sockaddr, &socksize) == -1) {
      int e = errno;
      ereport(WARNING, errmsg("getsockname failed with: %s", strerror(e)));
    }
    if (family == AF_INET) {
      Assert(addr.sin_family == AF_INET);
      *out_port = ntohs(addr.sin_port);
    } else if (family == AF_INET6) {
      Assert(addr.sin_family == AF_INET6);
      *out_port = ntohs(addr6.sin6_port);
    } else {
      return -1;
    }
  }

  return fd;
}

static int create_listener(int fd, listener_ctx *listener_ctx) {
  h2o_socket_t *sock;

  if (fd == -1) {
    return -1;
  }

  sock = h2o_evloop_socket_create(worker_event_loop, fd, H2O_SOCKET_FLAG_DONT_READ);
  sock->data = listener_ctx;
  listener_ctx->socket = sock;
  h2o_socket_read_start(sock, on_accept);

  return 0;
}

static h2o_pathconf_t *register_handler(h2o_hostconf_t *hostconf, const char *path,
                                        int (*on_req)(h2o_handler_t *, h2o_req_t *)) {
  h2o_pathconf_t *pathconf = h2o_config_register_path(hostconf, path, 0);
  h2o_handler_t *request_handler = h2o_create_handler(pathconf, sizeof(h2o_handler_t));
  request_handler->on_req = on_req;
  return pathconf;
}

// This must happen BEFORE signals are unblocked because of handler_receiver setup
static void setup_server() {
  h2o_hostconf_t *hostconf;

  h2o_config_init(&config);
  config.server_name = h2o_iovec_init(H2O_STRLIT("omni_httpd-" EXT_VERSION));
  hostconf = h2o_config_register_host(&config, h2o_iovec_init(H2O_STRLIT("default")), 65535);

  // Set up event loop for HTTP event loop
  worker_event_loop = h2o_evloop_create();
  event_loop_queue = h2o_multithread_create_queue(worker_event_loop);

  // Set up event loop for request handler loop
  handler_event_loop = h2o_evloop_create();
  handler_queue = h2o_multithread_create_queue(handler_event_loop);

  // This must happen BEFORE signals are unblocked
  h2o_multithread_register_receiver(handler_queue, &handler_receiver, on_message);

  h2o_pathconf_t *pathconf = register_handler(hostconf, "/", event_loop_req_handler);
}
static cvec_fd_fd accept_fds(char *socket_name) {
  struct sockaddr_un address;
  int socket_fd;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    int e = errno;
    ereport(ERROR, errmsg("can't create sharing socket"), errdetail(strerror(e)));
  }

  int err = fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK);
  if (err != 0) {
    ereport(ERROR, errmsg("Error setting O_NONBLOCK: %s", strerror(err)));
  }

  memset(&address, 0, sizeof(struct sockaddr_un));

  address.sun_family = AF_UNIX;
  snprintf(address.sun_path, sizeof(address.sun_path), "%s", socket_name);

try_connect:
  if (connect(socket_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) != 0) {
    int e = errno;
    if (e == EAGAIN || e == EWOULDBLOCK || e == ECONNREFUSED || e == ECONNRESET) {
      if (atomic_load(&worker_reload) || !atomic_load(&worker_running)) {
        // Don't try to get fds, roll with the reload
        return cvec_fd_fd_init();
      } else {
        goto try_connect;
      }
    }
    ereport(ERROR, errmsg("error connecting to sharing socket: %s", strerror(e)));
  }

  cvec_fd_fd result;

  do {
    errno = 0;
    if (atomic_load(&worker_reload) || !atomic_load(&worker_running)) {
      result = cvec_fd_fd_init();
      break;
    }
    result = recv_fds(socket_fd);
  } while (errno == EAGAIN || errno == EWOULDBLOCK);

  close(socket_fd);

  return result;
}

struct Router {
  Oid oid;
  int match_index;
  int handler_index;
  int priority_index;
};

static struct Router *routers = NULL;
static int NumRouters = 0;

struct Route {
  // Router it came from
  struct Router *router;
  // URLPattern match
  omni_httpd_urlpattern_t match;
  // HTTP method
  Oid method;
  // Route priority
  int priority;
  // Argument index for the tuple
  int tuple_index;
  // Argument index for the outcome
  int http_outcome_index;
  // Argument index for the request
  int http_request_index;
  // Argument index for the listener ID
  int listener_index;
  // Is this a handler or middleware?
  bool handler;
  // Does it return an outcome?
  bool returns_outcome;
  Datum tuple;
  Form_pg_proc proc;
};

static struct Route *routes = NULL;
static int NumRoutes = 0;
static int RouteCompare(const void *a1, const void *a2) {
  const struct Route *r1 = a1;
  const struct Route *r2 = a2;
  int ia = r1->priority;
  int ib = r2->priority;
  return (ib > ia) - (ib < ia);
}

static bool prepare_routers() {
  uint64 allocatedRoutes = 0;
  static MemoryContext RouterMemoryContext = NULL;
  if (RouterMemoryContext == NULL) {
    RouterMemoryContext =
        AllocSetContextCreate(TopMemoryContext, "RouterMemoryContext", ALLOCSET_DEFAULT_SIZES);
  } else {
    MemoryContextResetOnly(RouterMemoryContext);
  }
  bool result = true;
  SPI_connect();
  int rc = SPI_execute_plan(router_query, (Datum[0]){}, (char[0]){}, true, 0);
  if (rc != SPI_OK_SELECT) {
    ereport(WARNING, errmsg("failure retrieving routing tables"));
    result = false;
    goto cleanup;
  }

  // Reset routers
  if (routers) {
    routers = NULL;
    routes = NULL;
    NumRouters = 0;
    NumRoutes = 0;
  }

  int nvals = SPI_tuptable->numvals;
  routers = MemoryContextAlloc(RouterMemoryContext, sizeof(struct Router) * nvals);
  for (int i = 0; i < nvals; i++) {
    HeapTuple data = SPI_tuptable->vals[i];

    bool table_is_null = false;
    Datum taboid = SPI_getbinval(data, SPI_tuptable->tupdesc, 1, &table_is_null);

    bool match_index_is_null = false;
    Datum match_index = SPI_getbinval(data, SPI_tuptable->tupdesc, 2, &match_index_is_null);

    bool handler_index_is_null = false;
    Datum handler_index = SPI_getbinval(data, SPI_tuptable->tupdesc, 3, &handler_index_is_null);

    if (table_is_null || match_index_is_null || handler_index_is_null) {
      continue;
    }

    bool priority_index_is_null = false;
    Datum priority_index = SPI_getbinval(data, SPI_tuptable->tupdesc, 4, &priority_index_is_null);

    routers[NumRouters].oid = DatumGetObjectId(taboid);
    routers[NumRouters].match_index = DatumGetInt32(match_index);
    routers[NumRouters].handler_index = DatumGetInt32(handler_index);
    routers[NumRouters].priority_index = priority_index_is_null ? 0 : DatumGetInt32(priority_index);

    // Figure out the underlying table's type OID
    Oid tabtypoid = InvalidOid;
    HeapTuple tabtup = SearchSysCache1(RELOID, ObjectIdGetDatum(routers[NumRouters].oid));
    if (HeapTupleIsValid(tabtup)) {
      tabtypoid = ((Form_pg_class)GETSTRUCT(tabtup))->reltype;
    }
    ReleaseSysCache(tabtup);

    SPI_connect();

    bool found;
    RouterQueryHashEntry *entry = routerqueryhash_insert(routerqueryhash, taboid, &found);
    if (!found) {
      char *query =
          psprintf("select t, t.* from %s.%s t",
                   quote_identifier(get_namespace_name(get_rel_namespace(routers[NumRouters].oid))),
                   quote_identifier(get_rel_name(routers[NumRouters].oid)));

      entry->plan = SPI_prepare(query, 0, NULL);
      SPI_keepplan(entry->plan);
    }

    if (SPI_OK_SELECT == SPI_execute_plan(entry->plan, NULL, NULL, true, 0)) {

      if (SPI_gettypeid(SPI_tuptable->tupdesc, routers[NumRouters].match_index + 1) !=
          urlpattern_oid()) {
        SPI_finish();
        continue;
      }

      if (SPI_gettypeid(SPI_tuptable->tupdesc, routers[NumRouters].handler_index + 1) !=
          REGPROCEDUREOID) {
        SPI_finish();
        continue;
      }

      if (routers[NumRouters].priority_index > 0 &&
          SPI_gettypeid(SPI_tuptable->tupdesc, routers[NumRouters].priority_index + 1) !=
              route_priority_oid()) {
        SPI_finish();
        continue;
      }

      allocatedRoutes = allocatedRoutes + SPI_tuptable->numvals;
      if (routes == NULL) {
        routes = MemoryContextAlloc(RouterMemoryContext, sizeof(struct Route) * allocatedRoutes);
      } else {
        routes = repalloc(routes, sizeof(struct Route) * allocatedRoutes);
      }

      for (int j = 0; j < SPI_tuptable->numvals; j++) {
        bool match_is_null;
        Datum match_data = SPI_getbinval(SPI_tuptable->vals[j], SPI_tuptable->tupdesc,
                                         routers[NumRouters].match_index + 1, &match_is_null);
        bool handler_is_null;
        Datum handler_data = SPI_getbinval(SPI_tuptable->vals[j], SPI_tuptable->tupdesc,
                                           routers[NumRouters].handler_index + 1, &handler_is_null);
        if (match_is_null || handler_is_null) {
          continue;
        }

        Oid handler = DatumGetObjectId(handler_data);

        HeapTupleHeader match_tup = DatumGetHeapTupleHeader(match_data);

        /* Build a HeapTuple control structure */
        HeapTupleData tuple;
        tuple.t_len = HeapTupleHeaderGetDatumLength(match_tup);
        ItemPointerSetInvalid(&(tuple.t_self));
        tuple.t_tableOid = InvalidOid;
        tuple.t_data = match_tup;

        /* Get the tuple descriptor */
        TupleDesc tupleDesc = lookup_rowtype_tupdesc(HeapTupleHeaderGetTypeId(match_tup),
                                                     HeapTupleHeaderGetTypMod(match_tup));

        routes[NumRoutes].router = &routers[NumRouters];

        if (!priority_index_is_null) {
          bool priority_is_null;
          Datum priority_data =
              SPI_getbinval(SPI_tuptable->vals[j], SPI_tuptable->tupdesc,
                            routers[NumRouters].priority_index + 1, &priority_is_null);

          routes[NumRoutes].priority = priority_is_null ? 0 : DatumGetInt32(priority_data);
        } else {
          routes[NumRoutes].priority = 0;
        }

        MemoryContext oldcontext = MemoryContextSwitchTo(RouterMemoryContext);
        HeapTuple proc = SearchSysCacheCopy1(PROCOID, DatumGetObjectId(handler));
        MemoryContextSwitchTo(oldcontext);
        if (!HeapTupleIsValid(proc)) {
          continue;
        }
        routes[NumRoutes].proc = (Form_pg_proc)GETSTRUCT(proc);

        // Tuple index argument
        routes[NumRoutes].tuple_index = -1;
        routes[NumRoutes].http_request_index = -1;
        routes[NumRoutes].http_outcome_index = -1;
        routes[NumRoutes].listener_index = -1;
        routes[NumRoutes].returns_outcome =
            routes[NumRoutes].proc->prorettype == http_outcome_oid();

        {

          char **allargnames;

          Oid *allargtypes;
          char *allargmodes;
          routes[NumRoutes].handler = true;

          int numtypes = get_func_arg_info(proc, &allargtypes, &allargnames, &allargmodes);

          int argpos = 0;
          for (int arg = 0; arg < numtypes; arg++) {
            if (allargtypes[arg] == tabtypoid) {
              routes[NumRoutes].tuple_index = argpos;
            } else if (allargtypes[arg] == http_request_oid()) {
              routes[NumRoutes].http_request_index = argpos;
            } else if (allargtypes[arg] == http_outcome_oid()) {
              if (allargmodes[arg] == 'b') {
                routes[NumRoutes].http_outcome_index = argpos;
                routes[NumRoutes].handler = false;
              }
              if (allargmodes[arg] != 'i') {
                routes[NumRoutes].returns_outcome = true;
              }
            } else if (allargtypes[arg] == INT4OID) {
              routes[NumRoutes].listener_index = argpos;
            }
            if (!allargmodes || allargmodes[arg] != 'o') {
              argpos++;
            }
          }
        }

        {
          bool isnull;
          Datum route_tup = SPI_getbinval(SPI_tuptable->vals[j], SPI_tuptable->tupdesc, 1, &isnull);
          if (!isnull) {
            oldcontext = MemoryContextSwitchTo(RouterMemoryContext);
            routes[NumRoutes].tuple = datumTransfer(route_tup, false, -1);
            MemoryContextSwitchTo(oldcontext);
          }
        }
        // ReleaseSysCache

        omni_httpd_urlpattern_t *match = &routes[NumRoutes].match;

#define heap_match(index, name)                                                                    \
  {                                                                                                \
    bool attr_isnull;                                                                              \
    Datum d = heap_getattr(&tuple, index, tupleDesc, &attr_isnull);                                \
    if (attr_isnull) {                                                                             \
      match->name = NULL;                                                                          \
      match->name##_len = 0;                                                                       \
    } else {                                                                                       \
      MemoryContext o = MemoryContextSwitchTo(RouterMemoryContext);                                \
      text *t = DatumGetTextPCopy(d);                                                              \
      MemoryContextSwitchTo(o);                                                                    \
      match->name = VARDATA_ANY(t);                                                                \
      match->name##_len = VARSIZE_ANY_EXHDR(t);                                                    \
    }                                                                                              \
  }

        heap_match(1, protocol);
        heap_match(2, username);
        heap_match(3, password);
        heap_match(4, hostname);
        {
          bool attr_isnull;
          Datum d = heap_getattr(&tuple, 5, tupleDesc, &attr_isnull);
          if (attr_isnull) {
            match->port = 0;
          } else {
            match->port = DatumGetInt32(d);
          }
        }
        heap_match(6, pathname);
        heap_match(7, search);
        heap_match(8, hash);
        {
          bool attr_isnull;
          Datum d = heap_getattr(&tuple, 9, tupleDesc, &attr_isnull);
          if (attr_isnull) {
            routes[NumRoutes].method = InvalidOid;
          } else {
            routes[NumRoutes].method = DatumGetObjectId(d);
          }
        }

#undef heap_match

        NumRoutes++;

        ReleaseTupleDesc(tupleDesc);
      }
    }
    SPI_finish();
    //
    NumRouters++;
  }
  qsort(routes, NumRoutes, sizeof(struct Route), RouteCompare);
cleanup:
  SPI_finish();
  return result;
}

static int handler(handler_message_t *msg) {
  MemoryContext memory_context = CurrentMemoryContext;
  if (msg->type == handler_message_http) {
    if (msg->payload.http.msg.req == NULL) {
      // The connection is gone
      // We can release the message
      free(msg);
      goto release;
    }
  }

  SetCurrentStatementStartTimestamp();
  StartTransactionCommand();

  ActivePortal = execution_portal;

  bool succeeded = false;

  // Execute handler
  CurrentMemoryContext = HandlerContext;

  switch (msg->type) {
  case handler_message_http: {
    h2o_req_t *req = msg->payload.http.msg.req;
    listener_ctx *lctx = H2O_STRUCT_FROM_MEMBER(listener_ctx, context, req->conn->ctx);
    bool is_websocket_upgrade = msg->payload.http.websocket_upgrade;
    bool nulls[REQUEST_PLAN_PARAMS] = {[REQUEST_PLAN_METHOD] = false,
                                       [REQUEST_PLAN_PATH] = false,
                                       [REQUEST_PLAN_QUERY_STRING] = req->query_at == SIZE_MAX,
                                       [REQUEST_PLAN_BODY] = is_websocket_upgrade,
                                       [REQUEST_PLAN_HEADERS] = false};
    Oid method = InvalidOid;
    Datum values[REQUEST_PLAN_PARAMS] = {
        [REQUEST_PLAN_METHOD] = ({
          PointerGetDatum(cstring_to_text_with_len(req->method.base, req->method.len));
          Datum result = InvalidOid;
          for (int i = 0; i < http_method_last; i++) {
            if (strncmp(req->method.base, http_method_names[i], req->method.len) == 0) {
              method = http_method_oids[i];
              result = ObjectIdGetDatum(method);
              goto found;
            }
          }
          Assert(false);
        found:
          result;
        }),
        [REQUEST_PLAN_PATH] = PointerGetDatum(
            cstring_to_text_with_len(req->path_normalized.base, req->path_normalized.len)),
        [REQUEST_PLAN_QUERY_STRING] =
            req->query_at == SIZE_MAX
                ? PointerGetDatum(NULL)
                : PointerGetDatum(cstring_to_text_with_len(req->path.base + req->query_at + 1,
                                                           req->path.len - req->query_at - 1)),
        [REQUEST_PLAN_BODY] = ({
          bytea *result = NULL;
          if (is_websocket_upgrade) {
            goto done;
          }
          while (req->proceed_req != NULL) {
            req->proceed_req(req, NULL);
          }
          result = (bytea *)palloc(req->entity.len + VARHDRSZ);
          SET_VARSIZE(result, req->entity.len + VARHDRSZ);
          memcpy(VARDATA(result), req->entity.base, req->entity.len);
        done:
          PointerGetDatum(result);
        }),
        [REQUEST_PLAN_HEADERS] = ({
          TupleDesc header_tupledesc = TypeGetTupleDesc(http_header_oid(), NULL);
          BlessTupleDesc(header_tupledesc);

          // We are adding 1 because we prepend Omnigres-Connecting-IP
          // (see below)
          size_t headers_num = req->headers.size + 1;
          Datum *elems = (Datum *)palloc(sizeof(Datum) * headers_num);
          bool *header_nulls = (bool *)palloc(sizeof(bool) * headers_num);

          {
            // Provision 'Omnigres-Connecting-IP' header, as the first one
            // so that when `omni_http.http_header_get` is called, this is
            // the value we return; and it still preserves further attempts
            // to override
            struct sockaddr sa;
            Assert(req->conn->callbacks->get_peername);
            socklen_t socklen = req->conn->callbacks->get_peername(req->conn, &sa);
            char ip_str[INET6_ADDRSTRLEN] = "Unknown";

            switch (sa.sa_family) {
            case AF_INET: {
              struct sockaddr_in *addr_in = (struct sockaddr_in *)&sa;
              inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str));
              break;
            }
            case AF_INET6: {
              struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&sa;
              inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip_str, sizeof(ip_str));
              break;
            }
            default:
              break;
            }
            header_nulls[0] = 0;
            HeapTuple header_tuple =
                heap_form_tuple(header_tupledesc,
                                (Datum[2]){
                                    PointerGetDatum(cstring_to_text("omnigres-connecting-ip")),
                                    PointerGetDatum(cstring_to_text(ip_str)),
                                },
                                (bool[2]){false, false});
            elems[0] = HeapTupleGetDatum(header_tuple);
          }

          for (int i = 0; i < req->headers.size; i++) {
            h2o_header_t header = req->headers.entries[i];
            // We move the position by one to save space for Omnigres-Connecting-IP
            // as we prepended it
            int pos = i + 1;
            header_nulls[pos] = 0;
            HeapTuple header_tuple = heap_form_tuple(
                header_tupledesc,
                (Datum[2]){
                    PointerGetDatum(cstring_to_text_with_len(header.name->base, header.name->len)),
                    PointerGetDatum(cstring_to_text_with_len(header.value.base, header.value.len)),
                },
                (bool[2]){false, false});
            elems[pos] = HeapTupleGetDatum(header_tuple);
          }

          ArrayType *result =
              construct_md_array(elems, header_nulls, 1, (int[1]){headers_num}, (int[1]){1},
                                 http_header_oid(), -1, false, TYPALIGN_DOUBLE);
          PointerGetDatum(result);
        })};

    HeapTuple request_tuple = heap_form_tuple(request_tupdesc, values, nulls);

    Datum outcome;
    bool isnull = false;

    PG_TRY();
    {

      if (is_websocket_upgrade) {
        FmgrInfo flinfo;

        Oid function = websocket_handler_oid;
        fmgr_info(function, &flinfo);

        Snapshot snapshot = GetTransactionSnapshot();
        PushActiveSnapshot(snapshot);
        execution_portal->portalSnapshot = snapshot;

        LOCAL_FCINFO(fcinfo, 3);
        InitFunctionCallInfoData(*fcinfo, &flinfo, 3, InvalidOid /* collation */, NULL, NULL);

        fcinfo->args[0].value = Int32GetDatum(lctx->listener_id);
        fcinfo->args[0].isnull = false;
        fcinfo->args[1].value = UUIDPGetDatum((pg_uuid_t *)msg->payload.http.msg.ws_uuid);
        fcinfo->args[1].isnull = false;
        fcinfo->args[2].value = HeapTupleGetDatum(request_tuple);
        fcinfo->args[2].isnull = false;
        fcinfo->context = (fmNodePtr)non_atomic_call_context;
        outcome = FunctionCallInvoke(fcinfo);
        isnull = fcinfo->isnull;

        PopActiveSnapshot();
        execution_portal->portalSnapshot = NULL;

      } else {

        Snapshot snapshot = GetTransactionSnapshot();
        PushActiveSnapshot(snapshot);
        execution_portal->portalSnapshot = snapshot;

        prepare_routers();

        // If no handler will be selected, go for null answer (no data)
        isnull = true;
        int selected_handler = 0;

        for (int i = 0; i < NumRoutes; i++) {
          FmgrInfo flinfo;

          if (OidIsValid(routes[i].method) && routes[i].method != method) {
            continue;
          }

          if (!match_urlpattern(&routes[i].match, req->path.base, req->path.len)) {
            continue;
          }

          selected_handler++;

          fmgr_info(routes[i].proc->oid, &flinfo);

          int tuple_index = routes[i].tuple_index;
          int http_outcome_index = routes[i].http_outcome_index;
          int http_request_index = routes[i].http_request_index;
          int listener_index = routes[i].listener_index;

          int nargs = routes[i].proc->pronargs;

          LOCAL_FCINFO(fcinfo, 100); // max number of arguments
          InitFunctionCallInfoData(*fcinfo, &flinfo, nargs, InvalidOid /* collation */, NULL, NULL);

          // By default, all arguments are null
          for (int j = 0; j < nargs; j++) {
            fcinfo->args[j].isnull = true;
          }

          if (listener_index >= 0) {
            fcinfo->args[listener_index].value = Int32GetDatum(lctx->listener_id);
            fcinfo->args[listener_index].isnull = false;
          }
          if (http_request_index >= 0) {
            fcinfo->args[http_request_index].value = HeapTupleGetDatum(request_tuple);
            fcinfo->args[http_request_index].isnull = false;
          }
          if (http_outcome_index >= 0) {
            fcinfo->args[http_outcome_index].isnull =
                selected_handler == 1; // initial outcome is always null
            fcinfo->args[http_outcome_index].value = outcome;
          }
          if (tuple_index >= 0) {
            fcinfo->args[tuple_index].isnull = false;
            fcinfo->args[tuple_index].value = routes[i].tuple;
          }
          if (routes[i].proc->prokind == PROKIND_PROCEDURE) {
            fcinfo->context = (fmNodePtr)non_atomic_call_context;
          }
          Datum result = FunctionCallInvoke(fcinfo);
          isnull = fcinfo->isnull;
          if (routes[i].returns_outcome && !isnull) {
            if (routes[i].proc->prorettype == http_outcome_oid()) {
              outcome = result;
            } else if (routes[i].proc->prorettype == RECORDOID) {
              HeapTupleHeader th = DatumGetHeapTupleHeader(result);
              outcome = GetAttributeByNum(th, 1, &isnull);
            } else {
              isnull = true;
            }
          } else {
            isnull = true;
          }
          if (routes[i].handler) {
            break;
          }
        }

        PopActiveSnapshot();
        execution_portal->portalSnapshot = NULL;
      }
      heap_freetuple(request_tuple);
    }
    PG_CATCH();
    {
      heap_freetuple(request_tuple);
      MemoryContextSwitchTo(memory_context);
      WITH_TEMP_MEMCXT {
        ErrorData *error = CopyErrorData();
        ereport(WARNING, errmsg("Error executing query"),
                errdetail("%s: %s", error->message, error->detail));
      }

      FlushErrorState();

      req->res.status = 500;
      req->res.reason = http_status_reasons[req->res.status];
      h2o_queue_send_inline(&msg->payload.http.msg, H2O_STRLIT("Internal server error"));
      goto cleanup;
    }
    PG_END_TRY();
    switch (msg->type) {
    case handler_message_http: {
      succeeded = true;
      if (is_websocket_upgrade) {
        // Commit before sending a response
        CommitTransactionCommand();
        if (!isnull && DatumGetBool(outcome)) {
          h2o_queue_upgrade_to_websocket(&msg->payload.http.msg);
        } else {
          h2o_queue_abort(&msg->payload.http.msg);
        }

        break;
      } else if (!isnull) {
        // We know that the outcome is a variable-length type
        struct varlena *outcome_value = (struct varlena *)PG_DETOAST_DATUM_PACKED(outcome);

        VarSizeVariant *variant = (VarSizeVariant *)VARDATA_ANY(outcome_value);

        switch (variant->discriminant) {
        case HTTP_OUTCOME_RESPONSE: {
          HeapTupleHeader response_tuple = (HeapTupleHeader)&variant->data;

          // Status
          req->res.status = DatumGetUInt16(
              GetAttributeByIndex(response_tuple, HTTP_RESPONSE_TUPLE_STATUS, &isnull));
          if (isnull) {
            req->res.status = 200;
          }

          req->res.reason = http_status_reasons[req->res.status];

          bool content_length_specified = false;
          long long content_length = 0;

          // Headers
          Datum array_datum =
              GetAttributeByIndex(response_tuple, HTTP_RESPONSE_TUPLE_HEADERS, &isnull);
          if (!isnull) {
            ArrayType *headers = DatumGetArrayTypeP(array_datum);
            ArrayIterator iter = array_create_iterator(headers, 0, NULL);
            Datum header;
            while (array_iterate(iter, &header, &isnull)) {
              if (!isnull) {
                HeapTupleHeader header_tuple = DatumGetHeapTupleHeader(header);
                Datum name = GetAttributeByNum(header_tuple, 1, &isnull);
                if (!isnull) {
                  text *name_text = DatumGetTextPP(name);
                  size_t name_len = VARSIZE_ANY_EXHDR(name_text);
                  char *name_cstring = h2o_mem_alloc_pool(&req->pool, char *, name_len + 1);
                  text_to_cstring_buffer(name_text, name_cstring, name_len + 1);

                  Datum value = GetAttributeByNum(header_tuple, 2, &isnull);
                  if (!isnull) {
                    text *value_text = DatumGetTextPP(value);
                    size_t value_len = VARSIZE_ANY_EXHDR(value_text);
                    char *value_cstring = h2o_mem_alloc_pool(&req->pool, char *, value_len + 1);
                    text_to_cstring_buffer(value_text, value_cstring, value_len + 1);
                    if (name_len == sizeof("content-length") - 1 &&
                        strncasecmp(name_cstring, "content-length", name_len) == 0) {
                      // If we got content-length, we will not include it as we'll let h2o
                      // send the length.

                      // However, we'll remember the length set so that when we're processing the
                      // body, we can check its size and take action (reduce the size or send a
                      // warning if length specified is too big)

                      content_length_specified = true;
                      content_length = strtoll(value_cstring, NULL, 10);

                    } else {
                      // Otherwise, we'll just add the header
                      h2o_set_header_by_str(&req->pool, &req->res.headers, name_cstring, name_len,
                                            0, value_cstring, value_len, true);
                    }
                  }
                }
              }
            }
            array_free_iterator(iter);
          }

          Datum body = GetAttributeByIndex(response_tuple, HTTP_RESPONSE_TUPLE_BODY, &isnull);

          if (!isnull) {
            bytea *body_content = DatumGetByteaPP(body);
            size_t body_len = VARSIZE_ANY_EXHDR(body_content);
            if (content_length_specified) {
              if (body_len > content_length) {
                body_len = content_length;
              } else if (body_len < content_length) {
                ereport(WARNING, errmsg("Content-Length overflow"),
                        errdetail("Content-Length is set at %lld, but actual body is %zu",
                                  content_length, body_len));
              }
            }
            char *body_cstring = h2o_mem_alloc_pool(&req->pool, char *, body_len + 1);
            text_to_cstring_buffer(body_content, body_cstring, body_len + 1);
            // ensure we have the trailing \0 if we had to cut the response
            body_cstring[body_len] = 0;
            req->res.content_length = body_len;
            // Commit before sending a response
            CommitTransactionCommand();
            h2o_queue_send_inline(&msg->payload.http.msg, body_cstring, body_len);
          } else {
            // Commit before sending a response
            CommitTransactionCommand();
            h2o_queue_send_inline(&msg->payload.http.msg, "", 0);
          }
          break;
        }
        case HTTP_OUTCOME_ABORT: {
          // Commit before sending a response
          CommitTransactionCommand();
          h2o_queue_abort(&msg->payload.http.msg);
          break;
        }
        case HTTP_OUTCOME_PROXY: {
          HeapTupleHeader proxy_tuple = (HeapTupleHeader)&variant->data;

          // URL
          text *url =
              DatumGetTextPP(GetAttributeByIndex(proxy_tuple, HTTP_PROXY_TUPLE_URL, &isnull));
          if (isnull) {
            h2o_queue_abort(&msg->payload.http.msg);
            goto proxy_done;
          }
          // Preserve host
          int preserve_host = DatumGetBool(
              GetAttributeByIndex(proxy_tuple, HTTP_PROXY_TUPLE_PRESERVE_HOST, &isnull));
          if (isnull) {
            preserve_host = true;
          }
          size_t url_len = VARSIZE_ANY_EXHDR(url);
          char *url_cstring = h2o_mem_alloc_pool(&req->pool, char *, url_len + 1);
          text_to_cstring_buffer(url, url_cstring, url_len + 1);
          // Commit before sending a response
          CommitTransactionCommand();
          h2o_queue_proxy(&msg->payload.http.msg, url_cstring, preserve_host);
        proxy_done:
          break;
        }
        }
      } else {
        // Commit before sending a response
        CommitTransactionCommand();
        req->res.status = 204;
        req->res.reason = http_status_reasons[req->res.status];
        h2o_queue_send_inline(&msg->payload.http.msg, "", 0);
      }
      break;
    }
    default:
      Assert(false); // unhandled for now
      break;
    }
    break;
  }
  case handler_message_websocket_open:
  case handler_message_websocket_close:
    PG_TRY();
    {
      FmgrInfo flinfo;

      fmgr_info(msg->type == handler_message_websocket_open ? websocket_on_open_oid
                                                            : websocket_on_close_oid,
                &flinfo);
      LOCAL_FCINFO(fcinfo, 1);
      InitFunctionCallInfoData(*fcinfo, &flinfo, 1, InvalidOid /* collation */, NULL, NULL);

      fcinfo->args[0].value =
          UUIDPGetDatum((const pg_uuid_t *)(msg->type == handler_message_websocket_open
                                                ? msg->payload.websocket_open.uuid
                                                : msg->payload.websocket_close.uuid));
      fcinfo->args[0].isnull = false;
      fcinfo->context = (fmNodePtr)non_atomic_call_context;

      Snapshot snapshot = GetTransactionSnapshot();
      PushActiveSnapshot(snapshot);
      execution_portal->portalSnapshot = snapshot;

      FunctionCallInvoke(fcinfo);

      PopActiveSnapshot();
      execution_portal->portalSnapshot = NULL;
    }
    PG_CATCH();
    {
      MemoryContextSwitchTo(memory_context);
      WITH_TEMP_MEMCXT {
        ErrorData *error = CopyErrorData();
        const char *fn = msg->type == handler_message_websocket_open
                             ? "Error executing omni_httpd.websocket_on_open"
                             : "Error executing omni_httpd.websocket_on_close";
        ereport(WARNING, errmsg(fn), errdetail("%s: %s", error->message, error->detail));
      }

      FlushErrorState();

      goto cleanup;
    }
    PG_END_TRY();
    succeeded = true;
    CommitTransactionCommand();
    break;
  case handler_message_websocket_message:
    PG_TRY();
    {
      FmgrInfo flinfo;

      fmgr_info(msg->payload.websocket_message.opcode == WSLAY_TEXT_FRAME
                    ? websocket_on_message_text_oid
                    : websocket_on_message_binary_oid,
                &flinfo);
      LOCAL_FCINFO(fcinfo, 2);
      InitFunctionCallInfoData(*fcinfo, &flinfo, 2, InvalidOid /* collation */, NULL, NULL);

      fcinfo->args[0].value = UUIDPGetDatum((const pg_uuid_t *)msg->payload.websocket_message.uuid);
      fcinfo->args[0].isnull = false;
      fcinfo->args[1].value =
          PointerGetDatum(cstring_to_text_with_len((char *)msg->payload.websocket_message.message,
                                                   msg->payload.websocket_message.message_len));
      fcinfo->args[1].isnull = false;
      fcinfo->context = (fmNodePtr)non_atomic_call_context;

      Snapshot snapshot = GetTransactionSnapshot();
      PushActiveSnapshot(snapshot);
      execution_portal->portalSnapshot = snapshot;

      FunctionCallInvoke(fcinfo);

      PopActiveSnapshot();
      execution_portal->portalSnapshot = NULL;
    }
    PG_CATCH();
    {
      MemoryContextSwitchTo(memory_context);
      WITH_TEMP_MEMCXT {
        ErrorData *error = CopyErrorData();
        ereport(WARNING, errmsg("Error executing omni_httpd.on_websocket_message"),
                errdetail("%s: %s", error->message, error->detail));
      }

      FlushErrorState();

      goto cleanup;
    }
    PG_END_TRY();
    succeeded = true;
    CommitTransactionCommand();
    break;
  default:
    Assert(false);
  }

cleanup:
  // Ensure we no longer have an active portal
  ActivePortal = false;

  if (!succeeded) {
    AbortCurrentTransaction();
  }

  MemoryContextReset(HandlerContext);
release:

  return 0;
}
