/**
 * @file master_worker.c
 * @brief Master worker manages listening sockets and http workers
 *
 */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <access/xact.h>
#include <catalog/namespace.h>
#include <commands/async.h>
#include <common/int.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <port.h>
#include <postmaster/bgworker.h>
#include <postmaster/interrupt.h>
#include <storage/latch.h>
#include <storage/lmgr.h>
#include <storage/proc.h>
#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/inet.h>
#include <utils/json.h>
#include <utils/jsonb.h>
#include <utils/lsyscache.h>
#include <utils/memutils.h>
#include <utils/snapmgr.h>
#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include <h2o.h>

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
static cvec_fd sockets = {NULL};

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
    if (e != EPIPE) {
      ereport(WARNING, errmsg("error sending listening socket descriptor: %s", strerror(e)));
    }
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
    int e = errno;
    ereport(ERROR, errmsg("can't create sharing socket: %s", strerror(e)));
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

static volatile bool shutdown_worker = false;
void worker_shutdown() {
  shutdown_worker = true;
  close(socket_fd);
}

/**
 * @brief Flag indicating the need for the master worker to be reload
 *
 * It is important that it starts as `true` as we use it for the initial
 * configuration.
 */
static volatile bool worker_reload = true;

static pqsigfunc sigusr1_original_handler;
static void sigusr1_handler(int signo) {
  if (sigusr1_original_handler != NULL) {
    sigusr1_original_handler(signo);
  }
  if (notifyInterruptPending) {
    worker_reload = true;
  }
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

pg_atomic_uint32 *semaphore = NULL;

/**
 * @brief Master httpd worker
 *
 * @param db_oid Database OID
 */
void master_worker(Datum db_oid) {
  IsOmniHttpdWorker = true;

#if PG_MAJORVERSION_NUM >= 13
  pqsignal(SIGHUP, SignalHandlerForConfigReload);
#else
#warning "TODO: SignalHandlerForConfigReload for Postgres 12"
#endif
  pqsignal(SIGTERM, worker_shutdown);

  BackgroundWorkerUnblockSignals();
  BackgroundWorkerInitializeConnectionByOid(db_oid, InvalidOid, 0);

  // Ensure we don't exhaust max_worker_processes by waiting for any restarted omni_httpd workers to
  // terminate due to their "inattachment" to the master worker. If we are waiting for longer than
  // 10 seconds, proceed anyway.
  //
  // This functionality was manually tested by injecting a `sleep(1)` into http worker's startup
  // routine just before the termination based on the "inattachement" discovery, reporting finding a
  // matching "omni_httpd worker" worker through `ereport` and observing it.
  // The report observation can be made possible by enabling DEBUG2 logging:
  // ```sql
  // alter system set log_min_messages = 'DEBUG2';
  // select pg_reload_conf();
  // ```
  {
    struct timespec start, end;
    bool worker_present = true;
    clock_gettime(CLOCK_MONOTONIC, &start);
    bool *reported = palloc0(ProcGlobal->allProcCount * sizeof(bool));
    while (!shutdown_worker) {
      LWLockAcquire(ProcArrayLock, LW_SHARED);
      // We do this by inspecting all processes
      worker_present = false; // tentatively
      for (int i = 0; i < ProcGlobal->allProcCount; i++) {
        PGPROC proc = ProcGlobal->allProcs[i];
        pid_t pid = proc.pid;
        const char *worker_type = GetBackgroundWorkerTypeByPid(pid);
        if (worker_type &&
            strncmp(worker_type, "omni_httpd worker", sizeof("omni_httpd worker")) == 0 &&
            proc.databaseId == MyDatabaseId) {
          worker_present = true;
          if (!reported[i]) {
            ereport(DEBUG2,
                    errmsg("found auto-restarted omni_httpd worker %d, waiting for it to terminate",
                           pid));
            reported[i] = true;
          }
        }
      }
      LWLockRelease(ProcArrayLock);

      if (!worker_present) {
        break;
      }

      clock_gettime(CLOCK_MONOTONIC, &end);
      if (end.tv_sec - start.tv_sec > 10) {
        ereport(
            WARNING,
            errmsg("10 seconds has passed waiting for the termination of omni_httpd workers, "
                   "proceeding"),
            errdetail("omni_httpd workers were restarted and were expended to be terminated soon"),
            errhint("clean up older omni_httpd worker processes"));
        break;
      }

      (void)WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 100L,
                      PG_WAIT_EXTENSION);
      ResetLatch(MyLatch);
    }
    pfree(reported);
  }

  if (!BackendInitialized) {
    // Chances are, the extension was already dropped since this was not called.
    return;
  }

  char *tmp_path = mkdtemp(psprintf("%s/%s", *temp_dir, "omni_httpdXXXXXX"));
  if (tmp_path == NULL) {
    ereport(ERROR, errmsg("'%s' isn't a directory, please set omni_httpd.temp_dir to an existing "
                          "temporary directory",
                          *temp_dir));
  }
  socket_path = psprintf("%s/socket.%d", tmp_path, getpid());

#if PG_MAJORVERSION_NUM >= 18
  sigusr1_original_handler = procsignal_sigusr1_handler;
#else
  sigusr1_original_handler =
#endif
  pqsignal(SIGUSR1, sigusr1_handler);

  // Listen for configuration changes
  {
    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    PushActiveSnapshot(GetTransactionSnapshot());
    Async_Listen(OMNI_HTTPD_CONFIGURATION_NOTIFY_CHANNEL);
    PopActiveSnapshot();
    CommitTransactionCommand();
  }

  // Prepare the event loop
  event_loop = h2o_evloop_create();
  prepare_share_fd();

  cmap_portsock portsocks = cmap_portsock_init();
  cvec_bgwhandle http_workers = cvec_bgwhandle_init();

  bool http_workers_started = false;
  bool port_ready = false;

  if (master_worker_bgw == NULL || semaphore == NULL) {
    // Already being terminated
    return;
  }
  pg_atomic_write_u32(semaphore, 0);

  while (!shutdown_worker) {
    // Start the transaction
    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    PushActiveSnapshot(GetTransactionSnapshot());
    SPI_connect();

    // We clear this list every time to prepare an up-to-date version
    cvec_fd_clear(&sockets);

    Oid nspoid = get_namespace_oid("omni_httpd", false);
    Oid listenersoid = get_relname_relid("listeners", nspoid);

    // Get listeners
    while (worker_reload) {

      // Lock this table so that nobody can modify it
      // and therefore change the order of listeners (this is important for coordination of
      // the master worker with http workers) as they will use the order of listeners to align
      // with the vector of sockets sent.
      if (!ConditionalLockRelationOid(listenersoid, ExclusiveLock)) {
        // However, if we can't get the lock, see if ought to be shut down
        if (shutdown_worker) {
          // (and comply if we are)
          SPI_finish();
          PopActiveSnapshot();
          AbortCurrentTransaction();
          goto terminate;
        }
        // otherwise, try again
        continue;
        // (The big use case for conditionally locking here is that when we drop the extension,
        // it will lock extension objects exclusively and we're terminating master worker at the
        // final phase of the commit
      }

      worker_reload = false;

      if (SPI_execute(
              "select address, port, id, effective_port from omni_httpd.listeners order by id asc",
              false, 0) == SPI_OK_SELECT) {
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        SPITupleTable *tuptable = SPI_tuptable;
        cset_port ports = cset_port_init();
        // If there are no listeners, don't serve sockets (anymore), wait until
        // some will appear.
        if (SPI_processed == 0) {
          port_ready = false;
        }
        for (int i = 0; i < tuptable->numvals; i++) {
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
          bool current_effective_port_is_null = false;
          Datum current_effective_port =
              SPI_getbinval(tuple, tupdesc, 4, &current_effective_port_is_null);
          if (!port_is_null) {
            int port_no = DatumGetInt32(port);
            inet *inet_address = DatumGetInetPP(addr);
            char _address[MAX_ADDRESS_SIZE];
            char *address_str = pg_inet_net_ntop(ip_family(inet_address), ip_addr(inet_address),
                                                 ip_bits(inet_address), _address, sizeof(_address));

            const cmap_portsock_value *portsock = cmap_portsock_get(&portsocks, port_no);
          check_portsock:
            if (portsock == NULL) {
              // At least some ports are ready to be listened on
              port_ready = true;
              // Create a listening socket
              in_port_t effective_port;
              // This will be the listening socket
              int sock;
              // We will try listening on this port number. The reason it's separate from
              // `port_no` is that we might want to update the port number if the address was in use
              // (`EADDRINUSE`) but report the port change from the original port to the new one
              // and not from `0`
              int try_port_no = port_no;
            try_listen:
              sock = create_listening_socket(ip_family(inet_address) == PGSQL_AF_INET ? AF_INET
                                                                                      : AF_INET6,
                                             try_port_no, address_str, &effective_port);
              if (sock == -1) {
                if (errno == EADDRINUSE) {
                  // If there is a current effective port that is not the same as the one we're
                  // trying,
                  int effective_port_no = DatumGetInt32(current_effective_port);
                  if (!current_effective_port_is_null && effective_port_no != 0 &&
                      effective_port_no != try_port_no) {
                    // If it is an active listener,
                    portsock = cmap_portsock_get(&portsocks, effective_port_no);
                    if (portsock != NULL) {
                      port_no = effective_port_no;
                      goto check_portsock;
                    }
                    // otherwise, try listening on it again (this may have happened if we restarted)
                    try_port_no = effective_port_no;
                    // and ensure we don't try to do this again if it is no longer available
                    current_effective_port_is_null = true;
                    ereport(
                        WARNING,
                        errmsg("couldn't create listening socket on port %d: Address "
                               "already in use, falling back to currently used effective port %d",
                               port_no, try_port_no));
                  } else {
                    ereport(WARNING, errmsg("couldn't create listening socket on port %d: Address "
                                            "already in use, picking a different port",
                                            port_no));
                    // Let the operating system choose the port for us
                    try_port_no = 0;
                  }
                  goto try_listen;
                } else {
                  int e = errno;
                  ereport(WARNING, errmsg("couldn't create listening socket on port %d: %s",
                                          port_no, strerror(e)));
                }

              } else {
                if (effective_port != port_no) {
                  ereport(LOG, errmsg("omni_httpd listening port %d was replaced with %d", port_no,
                                      effective_port));
                }
                bool id_is_null = false;
                int update_retcode;
                if ((update_retcode = SPI_execute_with_args(
                         "update omni_httpd.listeners set effective_port = $1 where id = $2", 2,
                         (Oid[2]){INT4OID, INT4OID},
                         (Datum[2]){Int32GetDatum(effective_port),
                                    SPI_getbinval(tuple, tupdesc, 3, &id_is_null)},
                         "  ", false, 0)) != SPI_OK_UPDATE) {
                  ereport(WARNING, errmsg("can't update omni_httpd.listeners: %s",
                                          SPI_result_code_string(update_retcode)));
                }
                port_no = effective_port;
                cmap_portsock_insert(&portsocks, port_no, sock);
                cset_port_push(&ports, port_no);
                cvec_fd_push_back(&sockets, sock);
                ereport(LOG, errmsg("omni_httpd: Established listener on %d", port_no));
              }
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

      // If there is a port ready, don't wait anymore
      if (port_ready) {
        break;
      }

      // Unlock the table to allow for updates to it
      UnlockRelationOid(listenersoid, ExclusiveLock);

      // And then waiting
      (void)WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH, 1000L,
                      PG_WAIT_EXTENSION);
      ResetLatch(MyLatch);
      if (shutdown_worker) {
        SPI_finish();
        PopActiveSnapshot();
        AbortCurrentTransaction();
        goto terminate;
      }
    }
#if PG_MAJORVERSION_NUM >= 18
    ProcessMainLoopInterrupts();
#else
    HandleMainLoopInterrupts();
#endif

    // If HTTP workers have already been started, notify them of the change.
    // It is okay to notify them at this time as they will try to connect to the UNIX socket
    // which will be served as soon as we're done getting the new configuration and restart
    // the event loop.
    if (http_workers_started) {
      pg_atomic_write_u32(semaphore, 0);
      c_FOREACH(i, cvec_bgwhandle, http_workers) {
        pid_t pid;
        if (GetBackgroundWorkerPid(*i.ref, &pid) == BGWH_STARTED) {
          kill(pid, SIGUSR2);
        }
      }
    }

    // However, there's a problem with the fact that we don't know when all the http workers
    // are in fact in the "standby" mode, so when we commit this transaction, they may or may have
    // not even started their "standby"/"resume" process.
    //
    // So, we'll wait until all http workers have signalled their readiness, and exchange it back to
    // zero, to prepare for the next iteration.
    if (http_workers_started) {
      uint32 expected = cvec_bgwhandle_size(&http_workers);
      while (!pg_atomic_compare_exchange_u32(semaphore, &expected, 0)) {
        expected = cvec_bgwhandle_size(&http_workers);
#if PG_MAJORVERSION_NUM >= 18
        ProcessMainLoopInterrupts();
#else
        HandleMainLoopInterrupts();
#endif
        if (shutdown_worker) {
          SPI_finish();
          PopActiveSnapshot();
          AbortCurrentTransaction();
          goto terminate;
        }
      }
    }

    // When the configuration is loaded, insert a reload event
    // Next time http workers serve requests, they will be already using new data
    int insert_retcode =
        SPI_execute("insert into omni_httpd.configuration_reloads values (default)", false, 0);
    if (insert_retcode != SPI_OK_INSERT) {
      ereport(WARNING, errmsg("can't insert into omni_httpd.configuration_reloads: %s",
                              SPI_result_code_string(insert_retcode)));
    }

    // Unlock the table at last
    UnlockRelationOid(listenersoid, ExclusiveLock);

    // After doing this, we're ready to commit
    SPI_finish();
    PopActiveSnapshot();
    CommitTransactionCommand();

    // Start HTTP workers if they aren't already
    if (!http_workers_started) {
      BackgroundWorker worker = {
          .bgw_name = "omni_httpd worker",
          .bgw_type = "omni_httpd worker",
          .bgw_function_name = "http_worker",
          .bgw_notify_pid = MyProcPid,
          .bgw_main_arg = db_oid,
          // Restart these automatically so that the master worker doesn't
          // have to handle their restarts. If the workers are being restarted
          // en masse due to a crash of any other worker, they will no longer
          // receive bgw_notify_pid of this master worker and they will exit.
          .bgw_restart_time = 0,
          .bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
          .bgw_start_time = BgWorkerStart_RecoveryFinished};
      strncpy(worker.bgw_extra, socket_path, BGW_EXTRALEN - 1);
      strncpy(worker.bgw_library_name, MyBgworkerEntry->bgw_library_name,
              sizeof(worker.bgw_library_name) - 1);
      for (int i = 0; i < *num_http_workers; i++) {
        BackgroundWorkerHandle *handle;
        if (RegisterDynamicBackgroundWorker(&worker, &handle)) {
          pid_t worker_pid;
          BgwHandleStatus status = WaitForBackgroundWorkerStartup(handle, &worker_pid);

          switch (status) {
          case BGWH_POSTMASTER_DIED:
            goto terminate;
          case BGWH_STARTED: {
            cvec_bgwhandle_push(&http_workers, handle);
            break;
          }
          default:
            ereport(ERROR, (errcode(ERRCODE_INSUFFICIENT_RESOURCES),
                            errmsg("Could not start %s", worker.bgw_name)));
          }
        } else {
          ereport(WARNING, errcode(ERRCODE_INSUFFICIENT_RESOURCES),
                  errmsg("Could not start %s", worker.bgw_name),
                  errdetail("omni_httpd.http_workers = %d is higher than max_worker_processes = %d",
                            *num_http_workers, max_worker_processes),
                  errhint("Consider increasing max_worker_processes"));
        }
      }

      http_workers_started = true;
      // Ensure all workers have indeed started their loops
      uint32 expected = cvec_bgwhandle_size(&http_workers);
      ereport(LOG, errmsg("Started %d omni_httpd workers", expected));

      while (!pg_atomic_compare_exchange_u32(semaphore, &expected, 0)) {
        expected = cvec_bgwhandle_size(&http_workers);
#if PG_MAJORVERSION_NUM >= 18
        ProcessMainLoopInterrupts();
#else
        HandleMainLoopInterrupts();
#endif
        if (shutdown_worker) {
          goto terminate;
        }
      }
    }

    // Share the socket over a unix socket until terminated
    while (!shutdown_worker && !worker_reload && h2o_evloop_run(event_loop, INT32_MAX) == 0)
      ;
  }
terminate:
  if (http_workers_started) {
    ereport(LOG, errmsg("Shutting down HTTP workers"));
    // set set semaphore to INT32_MAX to signal normal termination
    // http_worker uses to determine if it should terminate with exit code
    // other than 0 (abnormal termination)
    pg_atomic_write_u32(semaphore, INT32_MAX);
    c_FOREACH(i, cvec_bgwhandle, http_workers) {
      pid_t pid;
      if (GetBackgroundWorkerPid(*i.ref, &pid) == BGWH_STARTED) {
        TerminateBackgroundWorker(*i.ref);
        WaitForBackgroundWorkerShutdown(*i.ref);
      }
    }
  }
  pg_atomic_write_u32(semaphore, 0);
  ereport(LOG, errmsg("Terminated omni_httpd"));
}
