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

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <access/xact.h>
#include <catalog/pg_authid.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <postmaster/bgworker.h>
#include <storage/latch.h>
#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/inet.h>
#include <utils/snapmgr.h>
#include <utils/syscache.h>
#include <utils/timestamp.h>
#if PG_MAJORVERSION_NUM >= 13
#include <postmaster/interrupt.h>
#endif

#include <metalang99.h>

#include <libpgaug.h>
#include <omni_sql.h>
#include <sum_type.h>

#include <dynpgext.h>

#include "event_loop.h"
#include "fd.h"
#include "http_worker.h"
#include "omni_httpd.h"

#if H2O_USE_LIBUV == 1
#error "only evloop is supported, ensure H2O_USE_LIBUV is not set to 1"
#endif

h2o_multithread_receiver_t handler_receiver;
h2o_multithread_queue_t *handler_queue;

static clist_listener_contexts listener_contexts;

static void on_message(h2o_multithread_receiver_t *receiver, h2o_linklist_t *messages) {
  while (!h2o_linklist_is_empty(messages)) {
    h2o_multithread_message_t *message =
        H2O_STRUCT_FROM_MEMBER(h2o_multithread_message_t, link, messages->next);

    request_message_t *request_msg = (request_message_t *)messages->next;

    handler(request_msg);

    h2o_linklist_unlink(&message->link);
  }
}

static void sigusr2() {
  atomic_store(&worker_reload, true);
  h2o_multithread_send_message(&event_loop_receiver, NULL);
  h2o_multithread_send_message(&handler_receiver, NULL);
}

static void sigterm() {
  atomic_store(&worker_running, false);
  h2o_multithread_send_message(&event_loop_receiver, NULL);
  h2o_multithread_send_message(&handler_receiver, NULL);
}

/**
 * HTTP worker starts with this user, and it's retrieved after initializing
 * the database.
 */
static Oid TopUser = InvalidOid;
/**
 * This is the current user used by the handler.
 *
 * Resets every time HTTP worker reloads configuration to allow for superuser
 * privileges during the reload.
 */
static Oid CurrentHandlerUser = InvalidOid;

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

  setup_server();

  // Block signals except for SIGUSR2 and SIGTERM
  pqsignal(SIGUSR2, sigusr2); // used to reload configuration
  pqsignal(SIGTERM, sigterm); // used to terminate the worker
  BackgroundWorkerUnblockSignals();

  // Start thread that will be servicing `worker_event_loop` and handling all
  // communication with the outside world. Current thread will be responsible for
  // configuration reloads and calling handlers.
  pthread_t event_loop_thread;
  event_loop_suspended = true;

  event_loop_register_receiver(); // This MUST happen before starting event_loop
  pthread_create(&event_loop_thread, NULL, event_loop, NULL);

  // Connect worker to the database
  BackgroundWorkerInitializeConnectionByOid(db_oid, InvalidOid, 0);
  TopUser = GetAuthenticatedUserId();

  listener_contexts = clist_listener_contexts_init();

  volatile pg_atomic_uint32 *semaphore =
      dynpgext_lookup_shmem(OMNI_HTTPD_CONFIGURATION_RELOAD_SEMAPHORE);
  Assert(semaphore != NULL);

  while (atomic_load(&worker_running)) {
    bool worker_reload_test = true;
    if (atomic_compare_exchange_strong(&worker_reload, &worker_reload_test, false)) {

      // Reset to TopUser
      SetUserIdAndSecContext(TopUser, 0);
      CurrentHandlerUser = TopUser;

      SetCurrentStatementStartTimestamp();
      StartTransactionCommand();
      PushActiveSnapshot(GetTransactionSnapshot());

      SPI_connect();

      // The idea here is that we get handlers sorted by listener id the same way they were
      // sorted in the master worker (`by id asc`) and since they will arrive in the same order
      // in the list of fds, we can simply get them by the index.
      //
      // This table is locked by the master worker and thus the order of listeners will not change

      int handlers_query_rc = SPI_execute(
          "select listeners.id, handlers.query, handlers.role_name "
          "from omni_httpd.listeners "
          "left join omni_httpd.listeners_handlers on listeners.id = "
          "listeners_handlers.listener_id "
          "left join omni_httpd.handlers handlers on handlers.id = listeners_handlers.handler_id "
          "order by listeners.id asc",
          false, 0);

      // Now that we have this information, we can let the master worker commit its update and
      // release the lock on the table.

      pg_atomic_add_fetch_u32(semaphore, 1);

      // When all HTTP workers will do the same, the master worker will start serving the
      // socket list.

      cvec_fd_fd fds = accept_fds(MyBgworkerEntry->bgw_extra);
      if (cvec_fd_fd_empty(&fds)) {
        SPI_finish();
        PopActiveSnapshot();
        AbortCurrentTransaction();
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
          if (iter.ref->plan != NULL) {
            SPI_freeplan(iter.ref->plan);
            iter.ref->plan = NULL;
          }

          if (iter.ref->socket != NULL) {
            h2o_socket_t *socket = iter.ref->socket;
            h2o_socket_export_t info;
            h2o_socket_export(socket, &info);
            h2o_socket_dispose_export(&info);
          }
          h2o_context_dispose(&iter.ref->context);
          MemoryContextDelete(iter.ref->memory_context);
          iter = clist_listener_contexts_erase_at(&listener_contexts, iter);
        next_ctx : {}
        }
      }

      // Set up new listeners as necessary
      c_FOREACH(i, cvec_fd_fd, new_fds) {
        int fd = (i.ref)->fd;

        listener_ctx c = {.fd = fd,
                          .master_fd = (i.ref)->master_fd,
                          .socket = NULL,
                          .plan = NULL,
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

      // Now we're ready to work with the results of the query we made earlier:
      if (handlers_query_rc == SPI_OK_SELECT) {

        // Here we have to track what was the last listener id
        int last_id = 0;
        // Here we have to track what was the last index
        int index = -1;

        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        SPITupleTable *tuptable = SPI_tuptable;
        for (int i = 0; i < tuptable->numvals; i++) {
          HeapTuple tuple = tuptable->vals[i];
          bool id_is_null = false;
          Datum id = SPI_getbinval(tuple, tupdesc, 1, &id_is_null);

          bool query_is_null = false;
          Datum query = SPI_getbinval(tuple, tupdesc, 2, &query_is_null);

          // Figure out socket index
          if (last_id != DatumGetInt32(id)) {
            last_id = DatumGetInt32(id);
            index++;
          }

          // If no handler has matched, continue to the next row
          if (query_is_null) {
            continue;
          }

          Oid role_id = InvalidOid;
          bool role_superuser = false;
          {
            bool role_name_is_null = false;
            Datum role_name = SPI_getbinval(tuple, tupdesc, 3, &role_name_is_null);

            HeapTuple roleTup = SearchSysCache1(AUTHNAME, role_name);
            if (!HeapTupleIsValid(roleTup)) {
              ereport(WARNING,
                      errmsg("role \"%s\" does not exist", NameStr(*DatumGetName(role_name))));
            } else {
              Form_pg_authid rform = (Form_pg_authid)GETSTRUCT(roleTup);
              role_id = rform->oid;
              role_superuser = rform->rolsuper;
            }
            ReleaseSysCache(roleTup);
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

          char *query_string = text_to_cstring(DatumGetTextPP(query));
          MemoryContext memory_context = CurrentMemoryContext;
          char *request_cte = psprintf(
              // clang-format off
              "select " "$" REQUEST_PLAN_PARAM(
                      REQUEST_PLAN_METHOD) "::text::omni_http.http_method AS method, "
              "$" REQUEST_PLAN_PARAM(REQUEST_PLAN_PATH) " as path, "
              "$" REQUEST_PLAN_PARAM(REQUEST_PLAN_QUERY_STRING) " as query_string, "
              "$" REQUEST_PLAN_PARAM(REQUEST_PLAN_BODY) " as body, "
              "$" REQUEST_PLAN_PARAM(REQUEST_PLAN_HEADERS) " as headers "
              // clang-format on
          );

          List *query_stmt = omni_sql_parse_statement(query_string);
          if (omni_sql_is_parameterized(query_stmt)) {
            ereport(WARNING,
                    errmsg("Listener query is parameterized and is rejected:\n %s", query_string));
          } else {
            List *request_cte_stmt = omni_sql_parse_statement(request_cte);
            query_stmt = omni_sql_add_cte(query_stmt, "request", request_cte_stmt, false, true);

            char *query = omni_sql_deparse_statement(query_stmt);
            list_free_deep(request_cte_stmt);
            list_free_deep(query_stmt);
            PG_TRY();
            {
              SPIPlanPtr plan = SPI_prepare(query, REQUEST_PLAN_PARAMS,
                                            (Oid[REQUEST_PLAN_PARAMS]){
                                                [REQUEST_PLAN_METHOD] = TEXTOID,
                                                [REQUEST_PLAN_PATH] = TEXTOID,
                                                [REQUEST_PLAN_QUERY_STRING] = TEXTOID,
                                                [REQUEST_PLAN_BODY] = BYTEAOID,
                                                [REQUEST_PLAN_HEADERS] = http_header_array_oid(),
                                            });

              Assert(plan != NULL);
              // Get role
              listener_ctx->role_id = role_id;
              listener_ctx->role_is_superuser = role_superuser;

              // We have to keep the plan as we're going to disconnect from SPI
              int keepret = SPI_keepplan(plan);
              if (keepret != 0) {
                ereport(WARNING, errmsg("Can't save plan: %s", SPI_result_code_string(keepret)));
                listener_ctx->plan = NULL;
              } else {
                listener_ctx->plan = plan;
              }
            }
            PG_CATCH();
            {
              MemoryContextSwitchTo(memory_context);
              WITH_TEMP_MEMCXT {
                ErrorData *error = CopyErrorData();
                ereport(WARNING, errmsg("Error preparing query %s", query),
                        errdetail("%s: %s", error->message, error->detail));
              }

              FlushErrorState();
            }
            PG_END_TRY();
            pfree(query);
          }
          pfree(query_string);
        }
      } else {
        ereport(WARNING, errmsg("Error fetching configuration: %s",
                                SPI_result_code_string(handlers_query_rc)));
      }

      SPI_finish();
      PopActiveSnapshot();
      AbortCurrentTransaction();
      cvec_fd_fd_drop(&fds);
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
}

static inline int listener_ctx_cmp(const listener_ctx *l, const listener_ctx *r) {
  return (l->plan == r->plan && l->socket == r->socket && l->fd == r->fd) ? 0 : -1;
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
  h2o_multithread_register_receiver(handler_queue, &handler_receiver, on_message);

  h2o_pathconf_t *pathconf = register_handler(hostconf, "/", event_loop_req_handler);
}
static cvec_fd_fd accept_fds(char *socket_name) {
  struct sockaddr_un address;
  int socket_fd;

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    ereport(ERROR, errmsg("can't create sharing socket"));
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
    if (e == EAGAIN || e == EWOULDBLOCK) {
      if (atomic_load(&worker_reload)) {
        // Don't try to get fds, roll with the reload
        return cvec_fd_fd_init();
      } else {
        goto try_connect;
      }
    }
    if (e == ECONNREFUSED) {
      goto try_connect;
    }
    ereport(ERROR, errmsg("error connecting to sharing socket: %s", strerror(e)));
  }

  cvec_fd_fd result;

  do {
    errno = 0;
    if (atomic_load(&worker_reload)) {
      result = cvec_fd_fd_init();
      break;
    }
    result = recv_fds(socket_fd);
  } while (errno == EAGAIN || errno == EWOULDBLOCK);

  close(socket_fd);

  return result;
}

static int handler(request_message_t *msg) {
  pthread_mutex_lock(&msg->mutex);
  h2o_req_t *req = msg->req;
  if (req == NULL) {
    // The connection is gone
    // We can release the message
    free(msg);
    goto release;
  }
  listener_ctx *lctx = H2O_STRUCT_FROM_MEMBER(listener_ctx, context, req->conn->ctx);

  SPIPlanPtr plan = lctx->plan;

  if (plan == NULL) {
    req->res.status = 500;
    h2o_queue_send_inline(msg, H2O_STRLIT("Internal server error!!"));
    goto release;
  }

  SetCurrentStatementStartTimestamp();
  StartTransactionCommand();
  PushActiveSnapshot(GetTransactionSnapshot());
  SPI_connect();

  bool succeeded = false;

  int ret;

  // If the current handler user is not the requested one,
  // set it to the requested one.
  if (CurrentHandlerUser != lctx->role_id) {
    CurrentHandlerUser = lctx->role_id;
    SetUserIdAndSecContext(CurrentHandlerUser,
                           lctx->role_is_superuser ? 0 : SECURITY_LOCAL_USERID_CHANGE);
  }

  // Execute listener's query
  MemoryContext memory_context = CurrentMemoryContext;
  PG_TRY();
  {
    char nulls[REQUEST_PLAN_PARAMS] = {[REQUEST_PLAN_METHOD] = ' ',
                                       [REQUEST_PLAN_PATH] = ' ',
                                       [REQUEST_PLAN_QUERY_STRING] =
                                           req->query_at == SIZE_MAX ? 'n' : ' ',
                                       [REQUEST_PLAN_BODY] = ' ',
                                       [REQUEST_PLAN_HEADERS] = ' '};
    ret = SPI_execute_plan(
        plan,
        (Datum[REQUEST_PLAN_PARAMS]){
            [REQUEST_PLAN_METHOD] =
                PointerGetDatum(cstring_to_text_with_len(req->method.base, req->method.len)),
            [REQUEST_PLAN_PATH] = PointerGetDatum(
                cstring_to_text_with_len(req->path_normalized.base, req->path_normalized.len)),
            [REQUEST_PLAN_QUERY_STRING] =
                req->query_at == SIZE_MAX
                    ? PointerGetDatum(NULL)
                    : PointerGetDatum(cstring_to_text_with_len(req->path.base + req->query_at + 1,
                                                               req->path.len - req->query_at - 1)),
            [REQUEST_PLAN_BODY] = ({
              while (req->proceed_req != NULL) {
                req->proceed_req(req, NULL);
              }
              bytea *result = (bytea *)palloc(req->entity.len + VARHDRSZ);
              SET_VARSIZE(result, req->entity.len + VARHDRSZ);
              memcpy(VARDATA(result), req->entity.base, req->entity.len);
              PointerGetDatum(result);
            }),
            [REQUEST_PLAN_HEADERS] = ({
              TupleDesc header_tupledesc = TypeGetTupleDesc(http_header_oid(), NULL);
              BlessTupleDesc(header_tupledesc);

              Datum *elems = (Datum *)palloc(sizeof(Datum) * req->headers.size);
              bool *header_nulls = (bool *)palloc(sizeof(bool) * req->headers.size);
              for (int i = 0; i < req->headers.size; i++) {
                h2o_header_t header = req->headers.entries[i];
                header_nulls[i] = 0;
                HeapTuple header_tuple =
                    heap_form_tuple(header_tupledesc,
                                    (Datum[2]){
                                        PointerGetDatum(cstring_to_text_with_len(header.name->base,
                                                                                 header.name->len)),
                                        PointerGetDatum(cstring_to_text_with_len(header.value.base,
                                                                                 header.value.len)),
                                    },
                                    (bool[2]){false, false});
                elems[i] = HeapTupleGetDatum(header_tuple);
              }
              ArrayType *result =
                  construct_md_array(elems, header_nulls, 1, (int[1]){req->headers.size},
                                     (int[1]){1}, http_header_oid(), -1, false, TYPALIGN_DOUBLE);
              PointerGetDatum(result);
            })},
        nulls, false, 1);
  }
  PG_CATCH();
  {
    MemoryContextSwitchTo(memory_context);
    WITH_TEMP_MEMCXT {
      ErrorData *error = CopyErrorData();
      ereport(WARNING, errmsg("Error executing query"),
              errdetail("%s: %s", error->message, error->detail));
    }

    FlushErrorState();

    req->res.status = 500;
    h2o_queue_send_inline(msg, H2O_STRLIT("Internal server error"));
    goto cleanup;
  }
  PG_END_TRY();
  int proc = SPI_processed;
  if (ret == SPI_OK_SELECT && proc > 0) {
    TupleDesc tupdesc = SPI_tuptable->tupdesc;
    SPITupleTable *tuptable = SPI_tuptable;
    HeapTuple tuple = tuptable->vals[0];

    int c = 1;
    for (int i = 0; i < tupdesc->natts; i++) {
      if (http_outcome_oid() == tupdesc->attrs[i].atttypid) {
        c = i + 1;
        break; // TODO: continue but issue a warning if another response is found?
      }
    }

    bool isnull = false;
    Datum outcome = SPI_getbinval(tuple, tupdesc, c, &isnull);
    if (!isnull) {
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
                    h2o_set_header_by_str(&req->pool, &req->res.headers, name_cstring, name_len, 0,
                                          value_cstring, value_len, true);
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
          h2o_queue_send_inline(msg, body_cstring, body_len);
        } else {
          h2o_queue_send_inline(msg, "", 0);
        }
        break;
      }
      case HTTP_OUTCOME_ABORT: {
        h2o_queue_abort(msg);
        break;
      }
      case HTTP_OUTCOME_PROXY: {
        HeapTupleHeader proxy_tuple = (HeapTupleHeader)&variant->data;

        // URL
        text *url = DatumGetTextPP(GetAttributeByIndex(proxy_tuple, HTTP_PROXY_TUPLE_URL, &isnull));
        if (isnull) {
          h2o_queue_abort(msg);
          goto proxy_done;
        }
        // Preserve host
        int preserve_host =
            DatumGetBool(GetAttributeByIndex(proxy_tuple, HTTP_PROXY_TUPLE_PRESERVE_HOST, &isnull));
        if (isnull) {
          preserve_host = true;
        }
        size_t url_len = VARSIZE_ANY_EXHDR(url);
        char *url_cstring = h2o_mem_alloc_pool(&req->pool, char *, url_len + 1);
        text_to_cstring_buffer(url, url_cstring, url_len + 1);
        h2o_queue_proxy(msg, url_cstring, preserve_host);
      proxy_done:
        break;
      }
      }
    } else {
      h2o_queue_send_inline(msg, "", 0);
    }
    succeeded = true;
  } else {
    if (ret == SPI_OK_SELECT) {
      // No result
      req->res.status = 204;
      h2o_queue_send_inline(msg, H2O_STRLIT(""));
      succeeded = true;
    } else {
      req->res.status = 500;
      ereport(WARNING, errmsg("Error executing query: %s", SPI_result_code_string(ret)));
      h2o_queue_send_inline(msg, H2O_STRLIT("Internal server error"));
    }
  }

cleanup:
  SPI_finish();
  PopActiveSnapshot();
  if (succeeded) {
    CommitTransactionCommand();
  } else {
    AbortCurrentTransaction();
  }

release:
  pthread_mutex_unlock(&msg->mutex);

  return 0;
}

h2o_socket_t *get_server_socket_from_req(h2o_req_t *req) {
  listener_ctx *lctx = H2O_STRUCT_FROM_MEMBER(listener_ctx, context, req->conn->ctx);
  return lctx->socket;
}