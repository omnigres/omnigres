/**
 * @file omni_httpd.c
 * @brief Extension initialization and exported functions
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

#include <common/int.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <port.h>
#include <postmaster/bgworker.h>
#if PG_MAJORVERSION_NUM >= 13
#include <postmaster/interrupt.h>
#endif
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

PG_MODULE_MAGIC;
DYNPGEXT_MAGIC;

#ifndef EXT_VERSION
#error "Extension version (VERSION) is not defined!"
#endif

CACHED_OID(http_header);
CACHED_OID(http_method);
CACHED_OID(http_response);

int num_http_workers;

/**
 * @brief Initializes allocated shared latch
 *
 * @param latch uninitialized latch
 * @param data unused
 */
static void init_latch(Latch *latch, void *data) { InitSharedLatch(latch); }

/**
 * @brief Initializes allocated reload counter
 *
 * @param latch uninitialized latch
 * @param data unused
 */
static void init_counter(uint64_t *counter, void *data) {
  atomic_init((_Atomic uint64_t *)counter, 0);
}

void _Dynpgext_init(const dynpgext_handle *handle) {
  DefineCustomIntVariable("omni_httpd.http_workers", "Number of HTTP workers", NULL,
                          &num_http_workers, 10, 1, INT_MAX, PGC_SIGHUP, 0, NULL, NULL, NULL);
  // Allocates memory for the worker latch
  handle->allocate_shmem(handle, LATCH, sizeof(Latch), (void (*)(void *ptr, void *data))init_latch,
                         NULL, DYNPGEXT_SCOPE_DATABASE_LOCAL);
  // Allocates memory for the worker counter
  handle->allocate_shmem(handle, COUNTER, sizeof(uint64_t),
                         (void (*)(void *ptr, void *data))init_counter, NULL,
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

PG_FUNCTION_INFO_V1(reload_configuration);

/**
 * @brief Triggers to be called on configuration update
 *
 * @return Datum
 */
Datum reload_configuration(PG_FUNCTION_ARGS) {
  Latch *worker_latch = (Latch *)dynpgext_lookup_shmem(LATCH);
  if (worker_latch == NULL) {
    ereport(NOTICE, errmsg("omni_httpd hasn't been properly loaded"));
    PG_RETURN_BOOL(false);
  }
  pg_memory_barrier();
  _Atomic uint64_t *worker_counter = (_Atomic uint64_t *)dynpgext_lookup_shmem(COUNTER);
  uint64_t before_counter = atomic_fetch_add(worker_counter, 1);
  uint64_t after_counter = 0;

  while (after_counter <= before_counter) {
    SetLatch(worker_latch);
    if (worker_latch->owner_pid != 0) {
      kill(worker_latch->owner_pid, SIGUSR2);
    }
    after_counter = atomic_load(worker_counter);
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
                                     (Datum[3]){
                                         PointerGetDatum(cstring_to_text(name)),
                                         PointerGetDatum(cstring_to_text(value)),
                                         BoolGetDatum(append),
                                     },
                                     (bool[3]){false, false, false});

  ExpandedArrayHeader *eah = DatumGetExpandedArray(headers);

  int *lb = eah->lbound;

  int indx;

  if (pg_sub_s32_overflow(lb[0], 1, &indx))
    ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE), errmsg("integer out of range")));

  return array_set_element(EOHPGetRWDatum(&eah->hdr), /* subscripts */ 1, &indx,
                           HeapTupleGetDatum(header),
                           /* isnull */ false, -1, -1, false, TYPALIGN_INT);
}

PG_FUNCTION_INFO_V1(http_response);

Datum http_response(PG_FUNCTION_ARGS) {
  TupleDesc response_tupledesc = TypeGetTupleDesc(http_response_oid(), NULL);
  BlessTupleDesc(response_tupledesc);

  Oid body_element_type = get_fn_expr_argtype(fcinfo->flinfo, 2);

  Datum values[3] = {PG_GETARG_DATUM(0), PG_GETARG_DATUM(1), PG_GETARG_DATUM(2)};

  Jsonb *jb;
  switch (body_element_type) {
  case TEXTOID:
  case VARCHAROID:
  case CHAROID:
    values[1] = add_header(values[1], "content-type", "text/plain; charset=utf-8", false);
  case BYTEAOID:
    values[1] = add_header(values[1], "content-type", "application/octet-stream", false);
    break;
  case JSONBOID:
    jb = PG_GETARG_JSONB_P(2);
    char *out = JsonbToCString(NULL, &jb->root, VARSIZE(jb));
    values[2] = PointerGetDatum(cstring_to_text(out));
  case JSONOID:
    values[1] = add_header(values[1], "content-type", "text/json", false);
    break;
  default:
    ereport(ERROR, errmsg("Can't (yet) cast %s to bytea",
                          format_type_extended(body_element_type, -1, FORMAT_TYPE_ALLOW_INVALID)));
  }

  HeapTuple response = heap_form_tuple(response_tupledesc, values, (bool[3]){false, false, false});

  PG_RETURN_DATUM(HeapTupleGetDatum(response));
}