/**
 * @file master_worker.c
 * @brief Master worker manages listening sockets and http workers
 *
 */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <access/xact.h>
#include <common/int.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <port.h>
#include <postmaster/bgworker.h>
#include <postmaster/interrupt.h>
#include <storage/latch.h>
#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/inet.h>
#include <utils/json.h>
#include <utils/jsonb.h>
#include <utils/memutils.h>
#include <utils/snapmgr.h>
#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include <h2o.h>

#include <dynpgext.h>
#include <libpgaug.h>

#include <libgluepg_stc.h>

#include "fd.h"
#include "omni_httpd.h"

/**
 * @brief UNIX socket that is used to share rights to the listening sockets
 *
 */
static int socket_fd;

/**
 * @brief Path to the UNIX socket
 *
 * @see socket_fd
 */
static char *socket_path;

/**
 * @brief UNIX socket's address
 *
 * @see socket_fd
 */
static struct sockaddr_un address;

/**
 * @brief UNIX socket's address length
 *
 * @see socket_fd
 */
static socklen_t address_length;

/**
 * @brief Master worker's event loop
 *
 */
static h2o_evloop_t *event_loop;

/**
 * @brief List of currently active sockets
 *
 * Maintained and used by the master worker
 *
 */
static cvec_fd sockets;

/**
 * @brief UNIX socket connection accept handler
 *
 * @param listener
 * @param err
 * @see socket_fd
 */
static void on_accept(h2o_socket_t *listener, const char *err) {
  h2o_socket_t *sock;

  if (err != NULL) {
    return;
  }

  if ((sock = h2o_evloop_socket_accept(listener)) == NULL) {
    return;
  }
  int fd = h2o_socket_get_fd(sock);
  if (send_fds(fd, &sockets) != 0) {
    int e = errno;
    ereport(ERROR, errmsg("error sending listening socket descriptor: %s", strerror(e)));
  }
  h2o_socket_close(sock);
}

/**
 * @brief Prepares `socket_fd` for accepting on and attaches it to `event_loop`
 *
 */
void prepare_share_fd() {
  address_length = sizeof(address);

  socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    ereport(ERROR, errmsg("can't create sharing socket"));
  }

  int enable = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  memset(&address, 0, sizeof(struct sockaddr_un));
  address.sun_family = AF_UNIX;
  snprintf(address.sun_path, sizeof(address.sun_path), "%s", socket_path);

  if (bind(socket_fd, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) != 0) {
    int e = errno;
    ereport(ERROR, errmsg("bind() failed: %s", strerror(e)));
  }

  if (listen(socket_fd, SOMAXCONN) != 0) {
    int e = errno;
    ereport(ERROR, errmsg("listen() failed: %s", strerror(e)));
  }

  h2o_socket_t *sock;

  sock = h2o_evloop_socket_create(event_loop, socket_fd, H2O_SOCKET_FLAG_DONT_READ);
  h2o_socket_read_start(sock, on_accept);
}

static Latch *latch;

static bool shutdown_worker = false;
void worker_shutdown() {
  shutdown_worker = true;
  close(socket_fd);
  SetLatch(latch);
}

/**
 * @brief Flag indicating the need for the master worker to be reload
 *
 * It is important that it starts as `true` as we use it for the initial
 * configuration.
 */
static bool worker_reload = true;

/**
 * @brief Called by SIGUSR2 handler
 *
 */
void reload() {

  // This is to ensure that if the reload is requested during the initial wait, we're setting
  // the latch we're on
  SetLatch(latch);

  worker_reload = true;
}

#define i_key int
#define i_val int
#define i_tag portsock
#include <stc/cmap.h>

#define i_val BackgroundWorkerHandle *
#define i_tag bgwhandle
#include <stc/cvec.h>

#define i_val int
#define i_tag port
#include <stc/cset.h>
/**
 * @brief Master httpd worker
 *
 * @param db_oid Database OID
 */
void master_worker(Datum db_oid) {
  char socket_path_template[] = "omni_httpdXXXXXX";
  char *tmpname = mkdtemp(socket_path_template);
  socket_path = psprintf("%s/socket.%d", tmpname, getpid());

#if PG_MAJORVERSION_NUM >= 13
  pqsignal(SIGHUP, SignalHandlerForConfigReload);
#else
#warning "TODO: SignalHandlerForConfigReload for Postgres 12"
#endif
  pqsignal(SIGTERM, worker_shutdown);
  // NB: It is important to set up the reload handler before owning the latch
  // as reload_configuration trigger would use it if the latch shows as owned.
  pqsignal(SIGUSR2, reload);

  BackgroundWorkerUnblockSignals();
  BackgroundWorkerInitializeConnectionByOid(db_oid, InvalidOid, 0);

  latch = (Latch *)dynpgext_lookup_shmem(LATCH);
  OwnLatch(latch);

  _Atomic uint64_t *worker_counter = (_Atomic uint64_t *)dynpgext_lookup_shmem(COUNTER);

  // Start the transaction
  SetCurrentStatementStartTimestamp();
  StartTransactionCommand();
  PushActiveSnapshot(GetTransactionSnapshot());

  // Prepare the event loop
  event_loop = h2o_evloop_create();
  prepare_share_fd();

  sockets = cvec_fd_init();
  cmap_portsock portsocks = cmap_portsock_init();
  cvec_bgwhandle http_workers = cvec_bgwhandle_init();

  bool http_workers_started = false;
  bool port_ready = false;
  SPI_connect();

  while (!shutdown_worker) {
    // We clear this list every time to prepare an up-to-date version
    cvec_fd_clear(&sockets);

    // If HTTP workes have already been started, notify them of the upcoming change.
    // It is okay to notify them at this time as they will try to connect to the UNIX socket
    // which will be served as soon as we're done getting the new configuration and restart
    // the event loop.
    if (http_workers_started) {
      c_FOREACH(i, cvec_bgwhandle, http_workers) {
        pid_t pid;
        if (GetBackgroundWorkerPid(*i.ref, &pid) == BGWH_STARTED) {
          kill(pid, SIGUSR2);
        }
      }
    }

    // Get listeners
    while (worker_reload) {
      worker_reload = false;
      if (SPI_execute("SELECT listen.addr, listen.port FROM omni_httpd.listeners, "
                      "unnest(omni_httpd.listeners.listen) AS listen",
                      false, 0) == SPI_OK_SELECT) {
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        SPITupleTable *tuptable = SPI_tuptable;
        cset_port ports = cset_port_init();
        // If there are no listeners, don't serve sockets (anymore), wait until
        // some will appear.
        if (SPI_processed == 0) {
          port_ready = false;
        }
        for (int i = 0; i < SPI_processed; i++) {
          if (cvec_fd_size(&sockets) == MAX_N_FDS) {
            ereport(WARNING,
                    errmsg("Reached maximum number of fds limit (%d). This restriction "
                           "will be removed in the future. No more sockets will be created.",
                           MAX_N_FDS));
            break;
          }
          HeapTuple tuple = tuptable->vals[i];
          bool addr_is_null = false;
          Datum addr = SPI_getbinval(tuple, tupdesc, 1, &addr_is_null);
          bool port_is_null = false;
          Datum port = SPI_getbinval(tuple, tupdesc, 2, &port_is_null);
          if (!port_is_null) {
            int port_no = DatumGetInt32(port);
            inet *inet_address = DatumGetInetPP(addr);
            char _address[MAX_ADDRESS_SIZE];
            char *address_str = pg_inet_net_ntop(ip_family(inet_address), ip_addr(inet_address),
                                                 ip_bits(inet_address), _address, sizeof(_address));

            const cmap_portsock_value *portsock = cmap_portsock_get(&portsocks, port_no);
            if (portsock == NULL) {
              // At least some ports are ready to be listened on
              port_ready = true;
              // Create a listening socket
              int sock = create_listening_socket(
                  ip_family(inet_address) == PGSQL_AF_INET ? AF_INET : AF_INET6, port_no,
                  address_str);
              cmap_portsock_insert(&portsocks, port_no, sock);
              cset_port_push(&ports, port_no);
              cvec_fd_push_back(&sockets, sock);
              ereport(LOG, errmsg("omni_httpd: Established listener on %d", port_no));
            } else {
              // This socket already exists
              cset_port_push(&ports, port_no);
              cvec_fd_push_back(&sockets, portsock->second);
            }
          }
        }

        // Scan for sockets that no longer match any listeners and remove them
        cmap_portsock_iter iter = cmap_portsock_begin(&portsocks);
        while (iter.ref) {
          int port = iter.ref->first;
          int fd = iter.ref->second;
          if (!cset_port_contains(&ports, port)) {
            close(fd);
            ereport(LOG, errmsg("omni_httpd: Removed listener on %d", port));
            iter = cmap_portsock_erase_at(&portsocks, iter);
          } else {
            cmap_portsock_next(&iter);
          }
        }
        cset_port_drop(&ports);
      }
      if (port_ready) {
        break;
      }
      (void)WaitLatch(latch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 1000L,
                      PG_WAIT_EXTENSION);
      ResetLatch(latch);
      if (shutdown_worker) {
        SPI_finish();
        DisownLatch(latch);
        return;
      }
    }
    atomic_fetch_add(worker_counter, 1);
    HandleMainLoopInterrupts();

    // Start HTTP workers if they aren't already
    if (!http_workers_started) {
      BackgroundWorker worker = {.bgw_name = "omni_httpd worker",
                                 .bgw_type = "omni_httpd worker",
                                 .bgw_function_name = "http_worker",
                                 .bgw_notify_pid = getpid(),
                                 .bgw_main_arg = db_oid,
                                 .bgw_flags =
                                     BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
                                 .bgw_start_time = BgWorkerStart_RecoveryFinished};
      strncpy(worker.bgw_extra, socket_path, BGW_EXTRALEN - 1);
      strncpy(worker.bgw_library_name, MyBgworkerEntry->bgw_library_name, BGW_MAXLEN - 1);
      for (int i = 0; i < num_http_workers; i++) {
        BackgroundWorkerHandle *handle;
        RegisterDynamicBackgroundWorker(&worker, &handle);
        pid_t worker_pid;
        if (WaitForBackgroundWorkerStartup(handle, &worker_pid) == BGWH_POSTMASTER_DIED) {
          return;
        }
        cvec_bgwhandle_push(&http_workers, handle);
      }
      http_workers_started = true;
    }

    // Share the socket over a unix socket until terminated
    while (!shutdown_worker && !worker_reload && h2o_evloop_run(event_loop, INT32_MAX) == 0)
      ;
  }
  SPI_finish();
  AbortCurrentTransaction();
  DisownLatch(latch);
}