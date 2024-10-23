// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <common/hashfn.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/snapmgr.h>
#if PG_MAJORVERSION_NUM < 17
#include <utils/typcache.h>
#endif
#if PG_MAJORVERSION_NUM < 15
#include <access/xact.h>
#endif
#if PG_MAJORVERSION_NUM > 14
#include <common/pg_prng.h>
#endif
#include <nodes/pg_list.h>
#include <utils/memutils.h>

PG_MODULE_MAGIC;

static List *backoff_values;
static int32 retry_attempts = 0;
static int64 cap_sleep_microsecs = 10000;
static int64 base_sleep_microsecs = 1;

/**
 * The backoff should increase with each attempt.
 */
static int64 get_backoff(int64 cap, int64 base, int32 attempt) {
  int exp = Min(attempt, 30); // caps the exponent to avoid overflowing,
                              // as the user can control the # of
                              // attempts.
  return Min(cap, base * (1 << exp));
}

/**
 * Get the random jitter to avoid contention in the backoff. Uses the
 * process seed initialized in `InitProcessGlobals`.
 */
static float8 get_jitter() {
#if PG_MAJORVERSION_NUM > 14
  return pg_prng_double(&pg_global_prng_state);
#else
  return rand() / (RAND_MAX + 1.0);
#endif
}

/**
 * Implements the backoff + fitter approach
 * https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/
 */
static int64 backoff_jitter(int64 cap, int64 base, int32 attempt) {
  int64 ret = (int64)(get_jitter() * get_backoff(cap, base, attempt));
  return (ret > 0 ? ret : 1);
}

/**
 * Turns the value into something that can be consumed by
 * `pg_sleep`. The literal comes copied from there, to ensure the same
 * ratio.
 */
static float8 to_secs(int64 secs) { return (float8)secs / 1000000.0; }

typedef struct {
  char *stmt;
  uint32 status;
  SPIPlanPtr plan;
} PreparedStatementEntry;

#define SH_PREFIX preparedstmthash
#define SH_ELEMENT_TYPE PreparedStatementEntry
#define SH_KEY_TYPE char *
#define SH_KEY stmt
#define SH_HASH_KEY(tb, key) string_hash(key, strlen(key))
#define SH_EQUAL(tb, a, b) (strcmp(a, b) == 0)
#define SH_SCOPE static inline
#define SH_DECLARE
#define SH_DEFINE
#include <lib/simplehash.h>

static preparedstmthash_hash *stmthash = NULL;
static MemoryContext RetryPreparedStatementMemoryContext;

static inline void init_preparedstmthash() {
  stmthash = preparedstmthash_create(RetryPreparedStatementMemoryContext, 8192, NULL);
}

void _PG_init(void) {
  RetryPreparedStatementMemoryContext = AllocSetContextCreate(
      TopMemoryContext, "omni_txn: retry prepared statements", ALLOCSET_DEFAULT_SIZES);
  if (stmthash == NULL) {
    init_preparedstmthash();
  }
}

PG_FUNCTION_INFO_V1(retry);

Datum retry(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("transaction statements argument is required"));
  }
  if (IsTransactionBlock()) {
    ereport(ERROR, errmsg("can't be used inside of a transaction block"));
  }
  int max_attempts = 10;
  if (!PG_ARGISNULL(1)) {
    max_attempts = PG_GETARG_INT32(1);
  }
  bool collect_backoff_values = false;
  if (!PG_ARGISNULL(3)) {
    collect_backoff_values = PG_GETARG_BOOL(3);
  }

   // Timeout logic
  int64 timeout_microsecs = 30000000; // Example: 30 seconds timeout
  TimestampTz start_time = GetCurrentTimestamp(); // Get start time
  bool retry = true;
  retry_attempts = 0;
  // This indicates that `params` were set
  TupleDesc paramsTupDesc = NULL;
  // Default values are prepared for the case when no `params` are set
  int paramsCount = 0;
  Datum *paramsValues;
  Oid *paramsTypes;
  char *paramsNullChars;

  // If `params` is set
  if (!PG_ARGISNULL(4)) {
    Oid type_oid = get_fn_expr_argtype(fcinfo->flinfo, 4);
    if (type_oid == RECORDOID) {
      HeapTupleData paramsTuple;
      bool *paramsNulls;
      HeapTupleHeader td = DatumGetHeapTupleHeader(PG_GETARG_DATUM(4));

      Oid tuple_type_id = HeapTupleHeaderGetTypeId(td);
      int32 tuple_typmod = HeapTupleHeaderGetTypMod(td);

      paramsTupDesc = lookup_rowtype_tupdesc(tuple_type_id, tuple_typmod);

      paramsCount = paramsTupDesc->natts;

      paramsTuple.t_len = HeapTupleHeaderGetDatumLength(td);
      paramsTuple.t_data = td;

      paramsValues = palloc(sizeof(Datum) * paramsCount);
      paramsNulls = palloc(sizeof(bool) * paramsCount);
      paramsNullChars = palloc(sizeof(char) * paramsCount);
      paramsTypes = palloc(sizeof(Oid) * paramsCount);

      heap_deform_tuple(&paramsTuple, paramsTupDesc, paramsValues, paramsNulls);

      // Iterate through the attributes to check for correctness
      // and set up all appropriate variables
      for (int i = 0; i < paramsCount; i++) {
        Oid att_type_oid = TupleDescAttr(paramsTupDesc, i)->atttypid; // Attribute type OID
        if (att_type_oid == UNKNOWNOID) {
          ReleaseTupleDesc(paramsTupDesc);
          ereport(ERROR, errmsg("query parameter %d is of unknown type", i + 1));
        }
        paramsNullChars[i] = paramsNulls[i] ? 'n' : ' ';
        paramsTypes[i] = att_type_oid;
      }
    }
  }

  if (collect_backoff_values) {
    // make sure that we are not collecting backoff values from another
    // transaction. Frees up existing memory in order not to leak it from
    // existing calls.
    list_free(backoff_values);
    backoff_values = NIL;
  }
  text *stmts = PG_GETARG_TEXT_PP(0);
  char *cstmts = text_to_cstring(stmts);
  bool retry = true;
  retry_attempts = 0;
  while (retry) {
    XactIsoLevel =
        (!PG_ARGISNULL(2) && PG_GETARG_BOOL(2)) ? XACT_REPEATABLE_READ : XACT_SERIALIZABLE;
    SPI_connect_ext(SPI_OPT_NONATOMIC);
    MemoryContext current_mcxt = CurrentMemoryContext;
    PG_TRY();
    {
      bool found;
      PreparedStatementEntry *entry = preparedstmthash_insert(
          stmthash, MemoryContextStrdup(RetryPreparedStatementMemoryContext, cstmts), &found);
      if (!found) {
        entry->plan = SPI_prepare(cstmts, paramsCount, paramsTypes);
        SPI_keepplan(entry->plan);
      }
      SPI_execute_plan(entry->plan, paramsValues, paramsNullChars, false, 0);
      SPI_finish();
      retry = false;
    }
    PG_CATCH();
    {
      MemoryContextSwitchTo(current_mcxt);
      SPI_finish();
      ErrorData *err = CopyErrorData();
      if (err->sqlerrcode == ERRCODE_T_R_SERIALIZATION_FAILURE) {
        if (++retry_attempts <= max_attempts) {
          int64 backoff_with_jitter_in_microsecs =
              backoff_jitter(cap_sleep_microsecs, base_sleep_microsecs, retry_attempts);

          if (collect_backoff_values) {
            // make sure to store the backoff values in a way that
            // survives the end of the transaction so that the user can
            // call `retry_backoff_values` to retrieve the values for
            // debugging/testing of the backoff process itself.
            MemoryContextSwitchTo(TopMemoryContext);
            backoff_values = lappend_int(backoff_values, backoff_with_jitter_in_microsecs);
            // go back to the context where we were
            MemoryContextSwitchTo(current_mcxt);
          }
          // abort current transaction
          PopActiveSnapshot();
          AbortCurrentTransaction();
          // backoff a little to avoid contention
          // add to list
          DirectFunctionCall1(pg_sleep, Float8GetDatum(to_secs(backoff_with_jitter_in_microsecs)));
          // start a new transaction
          SetCurrentStatementStartTimestamp();
          StartTransactionCommand();
          PushActiveSnapshot(GetTransactionSnapshot());

        } else {
          // abort the attempt to run the code.
          if (paramsTupDesc) {
            ReleaseTupleDesc(paramsTupDesc);
          }
          ereport(ERROR, errcode(err->sqlerrcode),
                  errmsg("maximum number of retries (%d) has been attempted", max_attempts));
        }
      } else {
        retry_attempts = 0;
        PG_RE_THROW();
      }
      CHECK_FOR_INTERRUPTS();
      FlushErrorState();
    }

    PG_END_TRY();
  }

  if (paramsTupDesc) {
    ReleaseTupleDesc(paramsTupDesc);
  }
  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(current_retry_attempt);

Datum current_retry_attempt(PG_FUNCTION_ARGS) { PG_RETURN_INT32(retry_attempts); }

PG_FUNCTION_INFO_V1(retry_backoff_values);

/**
 * Returns the accumulated backoff values, in microseconds.
 */
Datum retry_backoff_values(PG_FUNCTION_ARGS) {
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  const ListCell *backoff_value_node;
  foreach (backoff_value_node, backoff_values) {
    int64 backoff_value_content = lfirst_int(backoff_value_node);
    Datum values[1] = {Int64GetDatumFast(backoff_value_content)};
    bool isnull[1] = {false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(retry_prepared_statements);

Datum retry_prepared_statements(PG_FUNCTION_ARGS) {
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  preparedstmthash_iterator iter;
  preparedstmthash_start_iterate(stmthash, &iter);
  PreparedStatementEntry *entry;
  while ((entry = preparedstmthash_iterate(stmthash, &iter))) {
    Datum values[1] = {PointerGetDatum(cstring_to_text(entry->stmt))};
    bool isnull[1] = {false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(reset_retry_prepared_statements);

Datum reset_retry_prepared_statements(PG_FUNCTION_ARGS) {
  // Free all allocated plans
  preparedstmthash_iterator iter;
  preparedstmthash_start_iterate(stmthash, &iter);
  PreparedStatementEntry *entry;
  while ((entry = preparedstmthash_iterate(stmthash, &iter))) {
    SPI_freeplan(entry->plan);
  }

  // Reset the context to free all statements and the hashtable
  MemoryContextReset(RetryPreparedStatementMemoryContext);
  init_preparedstmthash();
  PG_RETURN_VOID();
}

bool TimestampDifferenceExceeds(TimestampTz current, TimestampTz start, int64 timeout) {
  int64 elapsed = TimestampDifferenceExceeds(current, start); // Get elapsed time in microseconds
  return elapsed >= timeout;
}