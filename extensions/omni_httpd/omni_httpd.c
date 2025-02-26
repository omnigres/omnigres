/**
 * @file omni_httpd.c
 * @brief Extension initialization and exported functions
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

#include <catalog/pg_cast.h>
#include <catalog/pg_database.h>
#include <commands/dbcommands.h>
#include <common/int.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <port.h>
#include <postmaster/bgworker.h>
#include <utils/rel.h>
#if PG_MAJORVERSION_NUM >= 13
#include <postmaster/interrupt.h>
#endif
#include <commands/async.h>
#include <common/hashfn.h>
#include <storage/latch.h>
#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/guc_tables.h>
#include <utils/inet.h>
#include <utils/json.h>
#include <utils/jsonb.h>
#include <utils/memutils.h>
#include <utils/snapmgr.h>
#include <utils/uuid.h>

#if PG_MAJORVERSION_NUM >= 14

#include <utils/wait_event.h>

#else
#include <pgstat.h>
#endif

#include <utils/syscache.h>

#include <h2o.h>

#include <libpgaug.h>
#include <omni/omni_v0.h>

#include <libgluepg_stc.h>

#include <omni_sql.h>

#include "event_loop.h"
#include "fd.h"
#include "omni_httpd.h"

PG_MODULE_MAGIC;
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_httpd", .version = EXT_VERSION,
                 .identity = "7005f29d-22ef-4c81-aff5-975dac62ad36");

#ifndef EXT_VERSION
#error "Extension version (VERSION) is not defined!"
#endif

CACHED_OID(omni_http, http_header);
CACHED_OID(omni_http, http_method);
CACHED_OID(http_response);
CACHED_OID(http_request);
CACHED_OID(http_outcome);
CACHED_OID(urlpattern);
CACHED_OID(route_priority);

bool IsOmniHttpdWorker;

int *num_http_workers;

char **temp_dir;

static bool check_temp_dir(char **newval, void **extra, GucSource source) {
  struct stat st;

  // Make sure temp dir path isn't too long leaving some space for filenames under it
  if (strlen(*newval) + 64 >= MAXPGPATH) {
    GUC_check_errmsg("'%s' temp directory name is too long.", *newval);
    return false;
  }

  // Do a basic sanity check that the specified temp dir exists.
  if (stat(*newval, &st) != 0 || !S_ISDIR(st.st_mode)) {
    GUC_check_errmsg("'%s' temp directory does not exist.", *newval);
    return false;
  }

  // Make sure temp dir doesn't exist under data dir
  if (path_is_prefix_of_path(DataDir, *newval)) {
    GUC_check_errmsg("temp directory location should not be inside the data directory");
    return false;
  }

  return true;
}

StaticAssertDecl(sizeof(pid_t) == sizeof(pg_atomic_uint32),
                 "pid has to fit in perfectly into uint32");

static void init_httpd_master_worker_pid(void *ptr, void *data) {
  pg_atomic_write_u32((pg_atomic_uint32 *)ptr, 0);
}

omni_bgworker_handle *master_worker_bgw = NULL;

static int num_http_workers_holder;

struct control_t {
  bool ready;
  bool should_start;
  bool started;
  LWLock lock;
};

static struct control_t *control;

static void init_control(const omni_handle *handle, void *ptr, void *arg, bool allocated) {
  struct control_t *ctl = (struct control_t *)ptr;
  if (allocated) {
    ctl->should_start = true;
    ctl->started = false;
    ctl->ready = true;
    handle->register_lwlock(handle, &ctl->lock, "omni_httpd startup control", true);
  } else {
    while (!ctl->ready) {
      CHECK_FOR_INTERRUPTS();
    }
  }
}

static void init_semaphore(const omni_handle *handle, void *ptr, void *arg, bool allocated) {
  if (allocated) {
    pg_atomic_init_u32((pg_atomic_uint32 *)ptr, 0);
  }
}

bool is_template_database() {
  HeapTuple tuple = SearchSysCache1(DATABASEOID, ObjectIdGetDatum(MyDatabaseId));
  Assert(HeapTupleIsValid(tuple)); // should not happen

  Form_pg_database db = (Form_pg_database)GETSTRUCT(tuple);
  bool is_template = db->datistemplate;
  ReleaseSysCache(tuple);

  return is_template;
}

static void start_master_worker(const omni_handle *handle, omni_bgworker_handle *bgw_handle,
                                omni_timing timing) {

  // If this is a template database, we don't start
  if (is_template_database()) {
    return;
  }

  LWLockAcquire(&control->lock, LW_EXCLUSIVE);
  if (control->should_start && !control->started) {
    // Prepares and registers the main background worker
    BackgroundWorker bgw = {
        .bgw_name = "omni_httpd",
        .bgw_type = "omni_httpd",
        .bgw_function_name = "master_worker",
        // Never restart this one because if it is being terminated abnormally,
        // postmaster will re-initialize shared memory and the module will receive
        // a new handle, new switchboard and will start the master worker itself
        .bgw_restart_time = BGW_NEVER_RESTART,
        .bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
        .bgw_main_arg = MyDatabaseId,
        .bgw_notify_pid = MyProcPid,
        .bgw_start_time = BgWorkerStart_RecoveryFinished};
    strncpy(bgw.bgw_library_name, handle->get_library_name(handle),
            sizeof(bgw.bgw_library_name) - 1);

    handle->request_bgworker_start(handle, &bgw, bgw_handle,
                                   (omni_bgworker_options){.timing = timing});
    control->started = true;
  }
  LWLockRelease(&control->lock);
}

static void register_start_master_worker(const omni_handle *handle, void *ptr, void *arg,
                                         bool allocated) {
  start_master_worker(handle, (omni_bgworker_handle *)ptr, omni_timing_after_commit);
}

volatile bool BackendInitialized = false;

static const omni_handle *module_handle;

void _Omni_init(const omni_handle *handle) {
  module_handle = handle;
  BackendInitialized = true;

  IsOmniHttpdWorker = false;

  // Try to see if this GUC is already defined
  omni_guc_variable guc_temp_dir = {
      .name = "omni_httpd.temp_dir",
      .long_desc = "Temporary directory for omni_httpd",
      .type = PGC_STRING,
      .typed = {.string_val = {.boot_value = "/tmp", .check_hook = check_temp_dir}},
      .context = PGC_SIGHUP};
  handle->declare_guc_variable(handle, &guc_temp_dir);
  temp_dir = guc_temp_dir.typed.string_val.value;

  int default_num_http_workers = 10;
#ifdef _SC_NPROCESSORS_ONLN
  default_num_http_workers = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  // considering the omni_httpd master worker and the "omni startup" worker used by omni TODO:
  // revisit once omni_workers is set.
  int threshold = max_worker_processes - 2;
  if (default_num_http_workers > threshold)
    default_num_http_workers = threshold;

  omni_guc_variable guc_num_http_workers = {
      .name = "omni_httpd.http_workers",
      .long_desc = "Number of HTTP workers",
      .type = PGC_INT,
      .typed = {.int_val = {.boot_value = default_num_http_workers,
                            .min_value = 1,
                            .max_value = INT_MAX}},
      .context = PGC_SIGHUP};
  handle->declare_guc_variable(handle, &guc_num_http_workers);
  num_http_workers = guc_num_http_workers.typed.int_val.value;

  bool control_found;
  const char *control_mem_name = psprintf(OMNI_HTTPD_CONFIGURATION_CONTROL, MyDatabaseId);

  control = handle->allocate_shmem(handle, control_mem_name, sizeof(*control), init_control, NULL,
                                   &control_found);

  bool semaphore_found;
  const char *semaphore_mem_name =
      psprintf(OMNI_HTTPD_CONFIGURATION_RELOAD_SEMAPHORE, MyDatabaseId);

  semaphore = handle->allocate_shmem(handle, semaphore_mem_name, sizeof(pg_atomic_uint32),
                                     init_semaphore, NULL, &semaphore_found);

  bool worker_bgw_found;
  const char *master_worker_mem_name =
      psprintf(OMNI_HTTPD_MASTER_WORKER, get_database_name(MyDatabaseId));
  master_worker_bgw =
      handle->allocate_shmem(handle, master_worker_mem_name, sizeof(*master_worker_bgw),
                             register_start_master_worker, NULL, &worker_bgw_found);
}

static void stop_master_worker(bool immediate) {
  LWLockAcquire(&control->lock, LW_EXCLUSIVE);

  if (control->started && master_worker_bgw != NULL) {
    omni_bgworker_handle *bgw_handle = (omni_bgworker_handle *)MemoryContextAlloc(
        IsTransactionState() ? TopTransactionContext : TopMemoryContext,
        sizeof(*master_worker_bgw));
    memcpy(bgw_handle, master_worker_bgw, sizeof(*master_worker_bgw));

    module_handle->request_bgworker_termination(
        module_handle, bgw_handle,
        (omni_bgworker_options){.timing = immediate ? omni_timing_immediately
                                                    : omni_timing_after_commit});
  }

  control->should_start = false;
  control->started = false;
  LWLockRelease(&control->lock);
}

void _Omni_deinit(const omni_handle *handle) {
  BackendInitialized = false;
  bool found;

  // Request stoppage while we have the information about the master worker
  // and the controls. Will happen when this transaction commits (if there's any)
  stop_master_worker(false);

  const char *master_worker_mem_name =
      psprintf(OMNI_HTTPD_MASTER_WORKER, get_database_name(MyDatabaseId));

  handle->deallocate_shmem(handle, master_worker_mem_name, &found);
  master_worker_bgw = NULL;

  const char *semaphore_mem_name =
      psprintf(OMNI_HTTPD_CONFIGURATION_RELOAD_SEMAPHORE, MyDatabaseId);

  handle->deallocate_shmem(handle, semaphore_mem_name, &found);
  semaphore = NULL;

  handle->unregister_lwlock(handle, &control->lock);

  const char *control_mem_name = psprintf(OMNI_HTTPD_CONFIGURATION_CONTROL, MyDatabaseId);

  handle->deallocate_shmem(handle, control_mem_name, &found);
  control = NULL;
}

PG_FUNCTION_INFO_V1(reload_configuration);

/**
 * @brief Triggers to be called on configuration update
 *
 * @return Datum
 */
Datum reload_configuration(PG_FUNCTION_ARGS) {
  // It is important to ensure that when we update the configuration (for example,
  // effective_port) in the background worker, we should not notify the background
  // worker of the change.
  if (!IsOmniHttpdWorker) {
    Async_Notify(OMNI_HTTPD_CONFIGURATION_NOTIFY_CHANNEL, NULL);
  }

  if (CALLED_AS_TRIGGER(fcinfo)) {
    return PointerGetDatum(((TriggerData *)(fcinfo->context))->tg_newtuple);
  } else {
    PG_RETURN_BOOL(true);
  }
}

/**
 * @brief Adds or appends a header
 *
 * @param headers 1-dim array of http_header
 * @param name header name
 * @param value header value
 * @param append append if true, otherwise set
 * @return new 1-dim array of http_header with the new http_header prepended.
 */
static inline Datum add_header(Datum headers, char *name, char *value, bool append) {
  TupleDesc header_tupledesc = TypeGetTupleDesc(http_header_oid(), NULL);
  BlessTupleDesc(header_tupledesc);

  HeapTuple header = heap_form_tuple(header_tupledesc,
                                     (Datum[2]){
                                         PointerGetDatum(cstring_to_text(name)),
                                         PointerGetDatum(cstring_to_text(value)),
                                     },
                                     (bool[2]){false, false});

  // If there are no headers yet
  if (headers == 0) {
    // simpyl construct a new array with one element
    return PointerGetDatum(construct_md_array((Datum[1]){HeapTupleGetDatum(header)},
                                              (bool[1]){false}, 1, (int[1]){1}, (int[1]){1},
                                              http_header_oid(), -1, false, TYPALIGN_INT));
  }

  ExpandedArrayHeader *eah = DatumGetExpandedArray(headers);

  int *lb = eah->lbound;
  int *dimv = eah->dims;

  int indx;

  if (pg_add_s32_overflow(lb[0], dimv[0], &indx))
    ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE), errmsg("integer out of range")));

  return array_set_element(EOHPGetRWDatum(&eah->hdr), 1, &indx, HeapTupleGetDatum(header), false,
                           -1, -1, false, eah->typalign);
}

PG_FUNCTION_INFO_V1(http_response);

Datum http_response(PG_FUNCTION_ARGS) {
#define ARG_BODY 0
#define ARG_STATUS 1
#define ARG_HEADERS 2
  TupleDesc response_tupledesc = TypeGetTupleDesc(http_response_oid(), NULL);
  BlessTupleDesc(response_tupledesc);

  Datum status = PG_GETARG_INT32(ARG_STATUS);
  if (PG_ARGISNULL(ARG_STATUS)) {
    status = 200;
  }

  Datum headers = PG_GETARG_DATUM(ARG_HEADERS);

  if (PG_ARGISNULL(ARG_HEADERS)) {
    // Signal to add_header that there are no headers yet
    headers = (Datum)0;
  }

  Datum values[3] = {[HTTP_RESPONSE_TUPLE_STATUS] = status,
                     [HTTP_RESPONSE_TUPLE_HEADERS] = headers,
                     [HTTP_RESPONSE_TUPLE_BODY] = PG_GETARG_DATUM(ARG_BODY)};

  // Process body, infer content-type, etc.
  if (!PG_ARGISNULL(ARG_BODY)) {

    // If we are to infer content-type, check if there was content type
    // specified explicitly.
    bool has_content_type = false;
    // If there is a headers array at all
    if (values[HTTP_RESPONSE_TUPLE_HEADERS] != 0) {
      TupleDesc tupdesc = TypeGetTupleDesc(http_header_oid(), NULL);
      BlessTupleDesc(tupdesc);

      ArrayIterator it = array_create_iterator(DatumGetArrayTypeP(headers), 0, NULL);
      bool isnull = false;
      Datum value;
      while (array_iterate(it, &value, &isnull)) {
        if (!isnull) {
          HeapTupleHeader tuple = DatumGetHeapTupleHeader(value);
          Datum name = GetAttributeByNum(tuple, 1, &isnull);
          if (!isnull) {
            text *name_str = DatumGetTextPP(name);
            if (strncasecmp(VARDATA_ANY(name_str), "content-type", VARSIZE_ANY_EXHDR(name_str)) ==
                0) {
              has_content_type = true;
              break;
            }
          }
        }
      }
      array_free_iterator(it);
    }

    Oid body_element_type = get_fn_expr_argtype(fcinfo->flinfo, ARG_BODY);
    Jsonb *jb;
    switch (body_element_type) {
    case TEXTOID:
    case VARCHAROID:
    case CHAROID:
      if (!has_content_type) {
        values[HTTP_RESPONSE_TUPLE_HEADERS] =
            add_header(values[HTTP_RESPONSE_TUPLE_HEADERS], "content-type",
                       "text/plain; charset=utf-8", false);
      }
      break;
    case BYTEAOID:
      if (!has_content_type) {
        values[HTTP_RESPONSE_TUPLE_HEADERS] = add_header(
            values[HTTP_RESPONSE_TUPLE_HEADERS], "content-type", "application/octet-stream", false);
      }
      break;
    case JSONBOID:
      jb = DatumGetJsonbP(values[HTTP_RESPONSE_TUPLE_BODY]);
      char *out = JsonbToCString(NULL, &jb->root, VARSIZE(jb));
      values[HTTP_RESPONSE_TUPLE_BODY] = PointerGetDatum(cstring_to_text(out));
    case JSONOID:
      if (!has_content_type) {
        values[HTTP_RESPONSE_TUPLE_HEADERS] = add_header(values[HTTP_RESPONSE_TUPLE_HEADERS],
                                                         "content-type", "application/json", false);
      }
      break;
    default:
      ereport(ERROR,
              errmsg("Can't (yet) cast %s to bytea",
                     format_type_extended(body_element_type, -1, FORMAT_TYPE_ALLOW_INVALID)));
    }
  }

  HeapTuple response =
      heap_form_tuple(response_tupledesc, values,
                      (bool[3]){[HTTP_RESPONSE_TUPLE_STATUS] = false,
                                [HTTP_RESPONSE_TUPLE_HEADERS] =
                                    values[HTTP_RESPONSE_TUPLE_HEADERS] == 0 ? true : false,
                                [HTTP_RESPONSE_TUPLE_BODY] = PG_ARGISNULL(ARG_BODY)});

  HeapTuple cast_tuple = SearchSysCache2(CASTSOURCETARGET, ObjectIdGetDatum(http_response_oid()),
                                         ObjectIdGetDatum(http_outcome_oid()));
  Assert(HeapTupleIsValid(cast_tuple));
  Form_pg_cast cast_form = (Form_pg_cast)GETSTRUCT(cast_tuple);
  Oid cast_func_oid = cast_form->castfunc;
  Assert(cast_func_oid != InvalidOid);
  ReleaseSysCache(cast_tuple);

  PG_RETURN_DATUM(OidFunctionCall1(cast_func_oid, HeapTupleGetDatum(response)));
#undef ARG_STATUS
#undef ARG_HEADERS
#undef ARG_BODY
}

PG_FUNCTION_INFO_V1(handlers_query_validity_trigger);

Datum handlers_query_validity_trigger(PG_FUNCTION_ARGS) {
  if (CALLED_AS_TRIGGER(fcinfo)) {
    TriggerData *trigger_data = (TriggerData *)(fcinfo->context);
    TupleDesc tupdesc = trigger_data->tg_relation->rd_att;
    bool isnull;
    HeapTuple tuple = TRIGGER_FIRED_BY_UPDATE(trigger_data->tg_event) ? trigger_data->tg_newtuple
                                                                      : trigger_data->tg_trigtuple;

    Datum query = SPI_getbinval(tuple, tupdesc, 2, &isnull);
    if (isnull) {
      ereport(ERROR, errmsg("query can't be null"));
    }
    List *stmts = omni_sql_parse_statement(text_to_cstring(DatumGetTextPP(query)));
    if (list_length(stmts) != 1) {
      ereport(ERROR, errmsg("query can only contain one statement"));
    }
    List *request_cte = omni_sql_parse_statement(
        "SELECT NULL::omni_http.http_method AS method, NULL::text AS path, NULL::text AS "
        "query_string, NULL::bytea AS body, NULL::omni_http.http_header[] AS headers");
    omni_sql_add_cte(stmts, "request", request_cte, false, true);
    char *err;
    if (!omni_sql_is_valid(stmts, &err)) {
      ereport(ERROR, errmsg("invalid query"), errdetail("%s", err));
    }
    return PointerGetDatum(tuple);
  } else {
    ereport(ERROR, errmsg("can only be called as a trigger"));
  }
}

PG_FUNCTION_INFO_V1(unload);

Datum unload(PG_FUNCTION_ARGS) {
  ereport(WARNING, errmsg("unload has been deprecated and is a no-op now"));
  PG_RETURN_VOID();
}

Datum websocket_send(PG_FUNCTION_ARGS, int8 kind) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("socket can't be null"));
  }
  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("data can't be null"));
  }

  pg_uuid_t *uuid = PG_GETARG_UUID_P(0);
  struct varlena *payload = PG_GETARG_VARLENA_PP(1);

  int sock;
  struct sockaddr_un server_addr = {.sun_family = AF_UNIX};

  if (websocket_unix_socket_path(&server_addr, &uuid->data) < 0) {
    ereport(ERROR, errmsg("websocket_unix_socket_path"), errdetail("socket path too long"));
  }

  if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    const int e = errno;
    ereport(ERROR, errmsg("socket"), errdetail(strerror(e)));
  }

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) < 0) {
    const int e = errno;
    close(sock);
    ereport(ERROR, errmsg("connect"), errdetail(strerror(e)));
  }

  // Send a message to the server
  struct {
    int8_t kind;
    size_t length;
  } __attribute__((packed)) msg = {.kind = kind, .length = VARSIZE_ANY_EXHDR(payload)};

  struct iovec iov[2];
  iov[0].iov_base = &msg;
  iov[0].iov_len = sizeof(msg);
  iov[1].iov_base = VARDATA_ANY(payload);
  iov[1].iov_len = msg.length;

  struct msghdr sendhdr = {0};
  sendhdr.msg_iov = iov;
  sendhdr.msg_iovlen = 2;

  if (sendmsg(sock, &sendhdr, 0) < 0) {
    int e = errno;
    ereport(ERROR, errmsg("sendmsg"), errdetail(strerror(e)));
  }

  close(sock);
  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(websocket_send_text);
Datum websocket_send_text(PG_FUNCTION_ARGS) { return websocket_send(fcinfo, 0); }

PG_FUNCTION_INFO_V1(websocket_send_bytea);
Datum websocket_send_bytea(PG_FUNCTION_ARGS) { return websocket_send(fcinfo, 1); }

PG_FUNCTION_INFO_V1(stop);
Datum stop(PG_FUNCTION_ARGS) {
  bool immediate = false;
  if (!PG_ARGISNULL(0)) {
    immediate = PG_GETARG_BOOL(0);
  }

  stop_master_worker(immediate);

  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(start);
Datum start(PG_FUNCTION_ARGS) {
  bool immediate = false;
  if (!PG_ARGISNULL(0)) {
    immediate = PG_GETARG_BOOL(0);
  }

  LWLockAcquire(&control->lock, LW_EXCLUSIVE);
  control->should_start = true;
  LWLockRelease(&control->lock);

  start_master_worker(module_handle, master_worker_bgw,
                      immediate ? omni_timing_immediately : omni_timing_after_commit);
  PG_RETURN_VOID();
}
