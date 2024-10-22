/**
 * @file omni_ledger.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/genam.h>
#include <access/relation.h>
#include <access/skey.h>
#include <access/table.h>
#include <catalog/pg_class.h>
#include <catalog/pg_index.h>
#include <catalog/pg_namespace.h>
#include <commands/trigger.h>
#include <common/hashfn.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <storage/predicate.h>
#include <utils/fmgroids.h>
#include <utils/fmgrprotos.h>
#include <utils/rel.h>
#include <utils/snapmgr.h>
#include <utils/syscache.h>
#include <utils/uuid.h>

#include <omni/omni_v0.h>

PG_MODULE_MAGIC;

OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_ledger", .version = EXT_VERSION,
                 .identity = "2b60ef1e-757e-4955-8c84-c1c784f54a52");

static HTAB *accounts = NULL;

#define ACCOUNT_CACHE_ENABLED true

#define ACCOUNT_DEBITS_ALLOWED_TO_EXCEED_CREDITS 1 << 0
#define ACCOUNT_CREDITS_ALLOWED_TO_EXCEED_DEBITS 1 << 1
#define ACCOUNT_CLOSED 1 << 7
#define ACCOUNT_INVALID 0

typedef struct {
  pg_uuid_t account_id;
  pg_uuid_t ledger_id;
  uint64_t flags;
  TransactionId txid;
} Account;

static inline char *printable_account_id(const pg_uuid_t *id) {
  return DatumGetCString(DirectFunctionCall1(uuid_out, UUIDPGetDatum(id)));
}

static TransactionId *invalidation;
static bool invalidation_announcement_pending = false;
static TransactionId observed_invalidation_announcement_at = InvalidTransactionId;

static void init_invalidation(const omni_handle *handle, void *ptr, void *data, bool allocated) {
  if (allocated) {
    *((TransactionId *)ptr) = InvalidTransactionId;
  }
}

static void xact_callback(omni_hook_handle *handle, XactEvent event) {
  switch (event) {
  case XACT_EVENT_COMMIT:
  case XACT_EVENT_PARALLEL_COMMIT:
  case XACT_EVENT_ABORT:
  case XACT_EVENT_PARALLEL_ABORT:
    // We are ready to commit, publish invalidation
    if (invalidation_announcement_pending) {
      *invalidation = GetCurrentTransactionId();
      observed_invalidation_announcement_at = InvalidTransactionId;
    }
    invalidation_announcement_pending = false;
    break;
  default:
    break;
  }
}

void _Omni_init(const omni_handle *handle) {
  const HASHCTL info = {
      .hcxt = TopMemoryContext,
      .keysize = sizeof(pg_uuid_t),
      .entrysize = sizeof(Account),
      .hash = (HashValueFunc)hash_bytes,
  };
  accounts = hash_create("omni_ledger account cache", 1024, &info,
                         HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);
  bool found;
  invalidation = handle->allocate_shmem(handle, "invalidated", sizeof(*invalidation),
                                        init_invalidation, NULL, &found);

  omni_hook xact_hook = {.type = omni_hook_xact_callback,
                         .name = "omni_leger transaction hook",
                         .fn = {.xact_callback = xact_callback}};
  handle->register_hook(handle, &xact_hook);
}

static inline bool invalidated() {
  if (invalidation_announcement_pending) {
    return true;
  }
  TransactionId current = GetActiveSnapshot()->xmin;
  if (!TransactionIdIsValid(current) ||
      (TransactionIdIsValid(observed_invalidation_announcement_at) &&
       TransactionIdFollowsOrEquals(current, observed_invalidation_announcement_at))) {
    // No new transaction yet
    return false;
  }
  observed_invalidation_announcement_at = current;

  return TransactionIdPrecedes(*invalidation, current);
}

static Oid get_relation_oid(const char *relation_name, const char *namespace_name) {
  Oid namespace_oid;
  Oid relation_oid;
  HeapTuple tuple;

  // Get the namespace OID
  namespace_oid =
      GetSysCacheOid1(NAMESPACENAME, Anum_pg_namespace_oid, CStringGetDatum(namespace_name));
  if (!OidIsValid(namespace_oid)) {
    ereport(ERROR, (errcode(ERRCODE_UNDEFINED_SCHEMA),
                    errmsg("schema \"%s\" does not exist", namespace_name)));
  }

  // Search for the relation in the specified namespace
  tuple =
      SearchSysCache2(RELNAMENSP, CStringGetDatum(relation_name), ObjectIdGetDatum(namespace_oid));

  if (!HeapTupleIsValid(tuple)) {
    ereport(ERROR, (errcode(ERRCODE_UNDEFINED_TABLE),
                    errmsg("relation \"%s.%s\" does not exist", namespace_name, relation_name)));
  }

  relation_oid = ((Form_pg_class)GETSTRUCT(tuple))->oid;

  ReleaseSysCache(tuple);

  return relation_oid;
}

static Oid get_primary_key_index_oid(const char *relation_name, const char *namespace_name) {
  Oid namespace_oid;
  Oid relation_oid;
  Oid primary_key_index_oid = InvalidOid;
  HeapTuple tuple;
  Relation rel;
  ListCell *cell;
  List *index_oids;

  // Get the namespace OID
  namespace_oid =
      GetSysCacheOid1(NAMESPACENAME, Anum_pg_namespace_oid, CStringGetDatum(namespace_name));
  if (!OidIsValid(namespace_oid)) {
    ereport(ERROR, (errcode(ERRCODE_UNDEFINED_SCHEMA),
                    errmsg("schema \"%s\" does not exist", namespace_name)));
  }

  // Get the relation OID
  relation_oid = GetSysCacheOid2(RELNAMENSP, Anum_pg_class_oid, CStringGetDatum(relation_name),
                                 ObjectIdGetDatum(namespace_oid));
  if (!OidIsValid(relation_oid)) {
    ereport(ERROR, (errcode(ERRCODE_UNDEFINED_TABLE),
                    errmsg("relation \"%s.%s\" does not exist", namespace_name, relation_name)));
  }

  // Open the relation to get index OIDs
  rel = relation_open(relation_oid, AccessShareLock);

  // Get the list of index OIDs for the relation
  index_oids = RelationGetIndexList(rel);

  // Iterate through the list of index OIDs to find the primary key index
  foreach (cell, index_oids) {
    Oid index_oid = lfirst_oid(cell);

    tuple = SearchSysCache1(INDEXRELID, ObjectIdGetDatum(index_oid));
    if (!HeapTupleIsValid(tuple)) {
      continue;
    }

    Form_pg_index index_form = (Form_pg_index)GETSTRUCT(tuple);
    if (index_form->indisprimary) {
      primary_key_index_oid = index_oid;
      ReleaseSysCache(tuple);
      break;
    }

    ReleaseSysCache(tuple);
  }

  // Release the list of index OIDs
  list_free(index_oids);

  // Close the relation
  relation_close(rel, AccessShareLock);

  if (!OidIsValid(primary_key_index_oid)) {
    ereport(ERROR, (errcode(ERRCODE_UNDEFINED_OBJECT),
                    errmsg("primary key index for relation \"%s.%s\" does not exist",
                           namespace_name, relation_name)));
  }

  return primary_key_index_oid;
}

static uint64_t account_flags(HeapTuple tuple, TupleDesc desc) {
  uint64_t flags;
  bool isnull;
  flags = DatumGetBool(heap_getattr(tuple, 3, desc, &isnull))
              ? ACCOUNT_DEBITS_ALLOWED_TO_EXCEED_CREDITS
              : 0;
  flags |= DatumGetBool(heap_getattr(tuple, 4, desc, &isnull))
               ? ACCOUNT_CREDITS_ALLOWED_TO_EXCEED_DEBITS
               : 0;
  flags |= DatumGetBool(heap_getattr(tuple, 5, desc, &isnull)) ? ACCOUNT_CLOSED : 0;
  return flags;
}

Account *find_account(pg_uuid_t id) {

  static Oid accounts_rel_oid = InvalidOid;
  static Oid accounts_pkey_oid = InvalidOid;

  if (!OidIsValid(accounts_rel_oid)) {
    accounts_rel_oid = get_relation_oid("accounts", "omni_ledger");
    accounts_pkey_oid = get_primary_key_index_oid("accounts", "omni_ledger");
  }

  bool found;
  if (!ACCOUNT_CACHE_ENABLED || invalidated()) {
    // (Brutally coarse) invalidate the entire cache
    HASH_SEQ_STATUS status;
    hash_seq_init(&status, accounts);
    Account *acc;
    while ((acc = (Account *)hash_seq_search(&status))) {
      hash_search(accounts, &acc->account_id, HASH_REMOVE, &found);
    }
  }
  Account *acc = (Account *)hash_search(accounts, &id, HASH_ENTER, &found);
  if (found) {
    return acc;
  }
  Relation rel = table_open(accounts_rel_oid, AccessShareLock);

  ScanKeyData entry[1];
  ScanKeyInit(&entry[0], 1, BTEqualStrategyNumber, F_UUID_EQ, UUIDPGetDatum(&id));

  SysScanDesc scandesc = systable_beginscan(rel, accounts_pkey_oid, true, NULL, 1, entry);

  HeapTuple tuple = systable_getnext(scandesc);

  if (HeapTupleIsValid(tuple)) {
    bool isnull = false;
    acc->ledger_id = *DatumGetUUIDP(heap_getattr(tuple, 2, rel->rd_att, &isnull));
    acc->flags = account_flags(tuple, rel->rd_att);
    acc->txid = GetCurrentTransactionIdIfAny();
  } else {
    acc->flags = ACCOUNT_INVALID;
  }

  systable_endscan(scandesc);

  table_close(rel, AccessShareLock);

  return acc;
}

typedef struct {
  pg_uuid_t account_id;
  uint64 credited;
  uint64 debited;
} Balance;

static HTAB *balances;

PG_FUNCTION_INFO_V1(calculate_account_balances);
Datum calculate_account_balances(PG_FUNCTION_ARGS) {
  static CommandId last_command_id = InvalidCommandId;
  static TransactionId last_transaction_id = InvalidTransactionId;
  if (accounts == NULL) {
    ereport(ERROR, errmsg("omni extension is required for omni_ledger"));
  }

  CommandId command_id = GetCurrentCommandId(false);
  TransactionId transaction_id = GetCurrentTransactionIdIfAny();
  if (transaction_id != last_transaction_id || last_command_id != command_id || balances == NULL) {
    // if `balances` was already allocated, it will get
    // deallocated once `TopTransactionContext` is gone
    const HASHCTL info = {
        .hcxt = TopTransactionContext,
        .keysize = sizeof(pg_uuid_t),
        .entrysize = sizeof(Balance),
        .hash = (HashValueFunc)hash_bytes,
    };
    balances = hash_create("omni_ledger in-flight balances", 1024, &info,
                           HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);
    last_command_id = command_id;
    last_transaction_id = transaction_id;
  }

  if (!CALLED_AS_TRIGGER(fcinfo)) {
    ereport(ERROR, errmsg("must be called as a trigger"));
  }

  TriggerData *trigdata = (TriggerData *)fcinfo->context;

  if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event)) {
    TupleDesc tupdesc = trigdata->tg_relation->rd_att;
    HeapTuple tuple = trigdata->tg_trigtuple;
    bool isnull;
    pg_uuid_t debit = *DatumGetUUIDP(heap_getattr(tuple, 2, tupdesc, &isnull));
    pg_uuid_t credit = *DatumGetUUIDP(heap_getattr(tuple, 3, tupdesc, &isnull));
    if (DatumGetBool(DirectFunctionCall2(uuid_eq, UUIDPGetDatum(&debit), UUIDPGetDatum(&credit)))) {
      ereport(ERROR, errmsg("can't transfer from and to the same account"));
    }
    int64 amount = DatumGetInt64(heap_getattr(tuple, 4, tupdesc, &isnull));
    if ((find_account(debit)->flags & ACCOUNT_CLOSED) == ACCOUNT_CLOSED) {
      ereport(ERROR, errmsg("can't transfer from a closed account"),
              errdetail("account %s is closed", printable_account_id(&debit)));
    }
    if ((find_account(credit)->flags & ACCOUNT_CLOSED) == ACCOUNT_CLOSED) {
      ereport(ERROR, errmsg("can't transfer to a closed account"),
              errdetail("account %s is closed", printable_account_id(&credit)));
    }
    bool found;
    Balance *debit_balance = hash_search(balances, &debit, HASH_ENTER, &found);
    if (!found) {
      debit_balance->credited = 0;
      debit_balance->debited = 0;
    }
    Balance *credit_balance = hash_search(balances, &credit, HASH_ENTER, &found);
    if (!found) {
      credit_balance->credited = 0;
      credit_balance->debited = 0;
    }
    debit_balance->debited += amount;
    credit_balance->credited += amount;
    return PointerGetDatum(tuple);
  }
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(update_account_balances);

Datum update_account_balances(PG_FUNCTION_ARGS) {
  static SPIPlanPtr check_debit_plan = NULL;
  static SPIPlanPtr plan = NULL;

  if (XactIsoLevel != XACT_SERIALIZABLE) {
    ereport(ERROR, errmsg("Ledger transfers must be done in a serializable transaction"));
  }

  if (!CALLED_AS_TRIGGER(fcinfo)) {
    ereport(ERROR, errmsg("must be called as a trigger"));
  }

  TriggerData *trigdata = (TriggerData *)fcinfo->context;

  if (TRIGGER_FIRED_FOR_STATEMENT(trigdata->tg_event)) {
    SPI_connect();

    if (check_debit_plan == NULL) {
      char *query =
          "with posted as ("
          "insert into omni_ledger.account_balance_slots as ab "
          "(account_id, debited, credited, slot) values "
          "(omni_ledger.account_id($3), $2, $1, $4) "
          "on conflict (account_id, slot) do update set debited = excluded.debited + ab.debited, "
          "credited = "
          "excluded.credited "
          "+ ab.credited where ab.account_id = excluded.account_id and ab.slot = "
          "excluded.slot returning debited, credited) "
          "select coalesce(sum(ab.debited)::bigint, 0), "
          "coalesce(sum(ab.credited)::bigint, 0) "
          "from omni_ledger.account_balance_slots ab where "
          "account_id = omni_ledger.account_id($3)";
      check_debit_plan = SPI_prepare(query, 4, (Oid[4]){INT8OID, INT8OID, UUIDOID, INT4OID});
      if (check_debit_plan == NULL) {
        ereport(ERROR, errmsg("%s", SPI_result_code_string(SPI_result)));
      }
      SPI_keepplan(check_debit_plan);
    }

    if (plan == NULL) {
      char *query =
          "insert into omni_ledger.account_balance_slots as ab "
          "(account_id, debited, credited, slot) values "
          "(omni_ledger.account_id($3), $2, $1, $4) "
          "on conflict (account_id, slot) do update set debited = excluded.debited + ab.debited, "
          "credited = "
          "excluded.credited + ab.credited"
          " where ab.account_id = excluded.account_id and ab.slot = "
          "$4";
      plan = SPI_prepare(query, 4, (Oid[4]){INT8OID, INT8OID, UUIDOID, INT4OID});
      if (plan == NULL) {
        ereport(ERROR, errmsg("%s", SPI_result_code_string(SPI_result)));
      }
      SPI_keepplan(plan);
    }
#if PG_MAJORVERSION_NUM < 17
    int slot = MyBackendId;
#else
    int slot = MyProcNumber;
#endif

    HASH_SEQ_STATUS seq;
    hash_seq_init(&seq, balances);
    const Balance *bal;
    while ((bal = hash_seq_search(&seq)) != NULL) {
      // Lookup account
      Account *acc = find_account(bal->account_id);
      acc->txid = GetCurrentTransactionId();

      // Figure out if we need perform any checks
      bool debits_not_allowed_to_exceed_credits =
          (acc->flags & ACCOUNT_DEBITS_ALLOWED_TO_EXCEED_CREDITS) == 0;
      bool credits_are_not_allowed_to_exceed_credits =
          (acc->flags & ACCOUNT_CREDITS_ALLOWED_TO_EXCEED_DEBITS) == 0;
      bool need_to_check = (bal->debited > 0 && debits_not_allowed_to_exceed_credits) ||
                           (bal->credited > 0 && credits_are_not_allowed_to_exceed_credits);

      SPIPlanPtr picked_plan = need_to_check ? check_debit_plan : plan;
      SPI_execp(picked_plan,
                (Datum[4]){Int64GetDatum(bal->credited), Int64GetDatum(bal->debited),
                           UUIDPGetDatum(&bal->account_id), Int32GetDatum(slot)},
                "    ", 0);

      if (need_to_check) {
        bool isnull;
        int64 debited =
            DatumGetInt64(SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull)) +
            bal->debited;
        int64 credited =
            DatumGetInt64(SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 2, &isnull)) +
            bal->credited;
        if (debited > credited && debits_not_allowed_to_exceed_credits) {
          ereport(ERROR, errmsg("debit exceeds credit"),
                  errdetail("expected to post debit of %lu and credit of %lu to %s", debited,
                            credited, printable_account_id(&bal->account_id)),
                  errhint("This account's flags stipulate that debits can't exceed credits"));
        }

        if (credited > debited && credits_are_not_allowed_to_exceed_credits) {
          ereport(ERROR, errmsg("credit exceeds debit"),
                  errdetail("expected to post debit of %lu and credit of %lu to %s", debited,
                            credited, printable_account_id(&bal->account_id)),
                  errhint("This account's flags stipulate that credits can't exceed debits"));
        }
      }
    }
    hash_destroy(balances);
    balances = NULL;

    SPI_finish();
  }

  return PointerGetDatum(trigdata->tg_trigtuple);
}

PG_FUNCTION_INFO_V1(deny_operation_on_accounts);
Datum deny_operation_on_accounts(PG_FUNCTION_ARGS) {
  if (!CALLED_AS_TRIGGER(fcinfo)) {
    ereport(ERROR, errmsg("must be called as a trigger"));
  }

  TriggerData *trigdata = (TriggerData *)fcinfo->context;
  if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event)) {
    if (XactIsoLevel != XACT_SERIALIZABLE) {
      ereport(ERROR, errmsg("account closure must be done in a serializable transaction"));
    }

    TupleDesc tupdesc = trigdata->tg_relation->rd_att;
    HeapTuple tuple = trigdata->tg_trigtuple;
    HeapTuple new_tuple = trigdata->tg_newtuple;
    bool isnull;
    Datum ledger = heap_getattr(tuple, 2, tupdesc, &isnull);
    Datum new_ledger = heap_getattr(new_tuple, 2, tupdesc, &isnull);
    uint8_t flags = account_flags(tuple, tupdesc);
    uint8_t new_flags = account_flags(new_tuple, tupdesc);
    if (DatumGetBool(DirectFunctionCall2(uuid_eq, ledger, new_ledger)) && flags != new_flags &&
        (flags | ACCOUNT_CLOSED) == new_flags) {
      // Notify all backends of this change (rather coarsely for now)
      invalidation_announcement_pending = true;

      return PointerGetDatum(new_tuple);
    } else {
      ereport(ERROR, errmsg("Accounts are immutable with the exception of closure"));
    }
  }
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(transaction_affected_accounts);
Datum transaction_affected_accounts(PG_FUNCTION_ARGS) {
  TransactionId since_txn = GetCurrentTransactionIdIfAny();
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  if (!(accounts == NULL || !TransactionIdIsValid(since_txn))) {
    HASH_SEQ_STATUS status;
    hash_seq_init(&status, accounts);
    Account *acc;
    while ((acc = hash_seq_search(&status))) {
      if (TransactionIdEquals(acc->txid, since_txn)) {
        continue;
      }
      Datum values[2] = {UUIDPGetDatum(&acc->account_id), UUIDPGetDatum(&acc->ledger_id)};
      bool isnull[2] = {false, false};
      tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
    }
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(statement_affected_accounts);
Datum statement_affected_accounts(PG_FUNCTION_ARGS) {
  TransactionId since_txn = GetCurrentTransactionIdIfAny();
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  if (!(balances == NULL || !TransactionIdIsValid(since_txn))) {
    HASH_SEQ_STATUS status;
    hash_seq_init(&status, balances);
    Balance *bal;
    while ((bal = hash_seq_search(&status))) {
      Account *acc = find_account(bal->account_id);
      if (TransactionIdEquals(acc->txid, since_txn)) {
        continue;
      }
      Datum values[2] = {UUIDPGetDatum(&acc->account_id), UUIDPGetDatum(&acc->ledger_id)};
      bool isnull[2] = {false, false};
      tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
    }
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}
