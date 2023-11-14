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

#include <utils/syscache.h>

#include <h2o.h>

#include <dynpgext.h>
#include <libpgaug.h>

#include <libgluepg_stc.h>

#include <omni_sql.h>

#include "fd.h"
#include "omni_httpd.h"

PG_MODULE_MAGIC;
DYNPGEXT_MAGIC;

#ifndef EXT_VERSION
#error "Extension version (VERSION) is not defined!"
#endif

CACHED_OID(omni_http, http_header);
CACHED_OID(omni_http, http_method);
CACHED_OID(http_response);
CACHED_OID(http_outcome);

bool IsOmniHttpdWorker;

int num_http_workers;

static void init_semaphore(void *ptr, void *data) { pg_atomic_init_u32(ptr, 0); }

void _Dynpgext_init(const dynpgext_handle *handle) {
  int default_num_http_workers = 10;
#ifdef _SC_NPROCESSORS_ONLN
  default_num_http_workers = sysconf(_SC_NPROCESSORS_ONLN);
#endif
  {
    const char *http_workers_val = GetConfigOption("omni_httpd.http_workers", true, false);
    int http_workers_flag = GetConfigOptionFlags("omni_httpd.http_workers", true);
    if ((http_workers_val == NULL && !(http_workers_flag & GUC_CUSTOM_PLACEHOLDER)) ||
        (http_workers_val != NULL && http_workers_flag & GUC_CUSTOM_PLACEHOLDER)) {
      DefineCustomIntVariable("omni_httpd.http_workers", "Number of HTTP workers", NULL,
                              &num_http_workers, default_num_http_workers, 1, INT_MAX, PGC_SIGHUP,
                              0, NULL, NULL, NULL);
    }
  }

  handle->allocate_shmem(handle, OMNI_HTTPD_CONFIGURATION_RELOAD_SEMAPHORE,
                         sizeof(pg_atomic_uint32), init_semaphore, NULL,
                         DYNPGEXT_SCOPE_DATABASE_LOCAL);

  // Prepares and registers the main background worker
  BackgroundWorker bgw = {.bgw_name = "omni_httpd",
                          .bgw_type = "omni_httpd",
                          .bgw_function_name = "master_worker",
                          .bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION,
                          .bgw_start_time = BgWorkerStart_RecoveryFinished};
  strncpy(bgw.bgw_library_name, handle->library_name, BGW_MAXLEN);
  handle->register_bgworker(handle, &bgw, NULL, NULL,
                            DYNPGEXT_REGISTER_BGWORKER_NOTIFY | DYNPGEXT_SCOPE_DATABASE_LOCAL);
}

void _PG_init() { IsOmniHttpdWorker = false; }

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