/**
 * @file omni_var.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/xact.h>
#include <utils/builtins.h>
#include <utils/guc.h>
#include <utils/hsearch.h>
#include <utils/lsyscache.h>
#include <utils/memutils.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(set);

PG_FUNCTION_INFO_V1(get);

static TransactionId last_used_txnid = InvalidTransactionId;
static HTAB *current_tab = NULL;

typedef struct VariableValue {
  bool isnull;
  Oid oid;
  SubTransactionId subxactid;
  Datum value;
  struct VariableValue *previous;
} VariableValue;

typedef struct {
  NameData name;
  VariableValue variable_value;
} Variable;

static int nvars = 0;

void _PG_init() {
  DefineCustomIntVariable(
      "omni_var.estimated_initial_txn_variables_count", "Estimated number of transaction variables",
      "Estimated number of transaction variables, changes apply to the next transaction", &nvars,
      1024, 0, 65535, PGC_USERSET, 0, NULL, NULL, NULL);
}

#if PG_MAJORVERSION_NUM < 14
#define TXN_HASH_ELEM HASH_ELEM
#else
#define TXN_HASH_ELEM HASH_ELEM | HASH_STRINGS
#endif

static bool subtransaction_callback_registered = false;

static void subtransaction_callback(SubXactEvent event, SubTransactionId xact_id,
                                    SubTransactionId parent_xact_id, void *arg) {
  switch (event) {
  case SUBXACT_EVENT_ABORT_SUB:
    // Aborting subtransaction xact_id

    if (current_tab == NULL) {
      // Nothing to process, bail
      return;
    }
    HASH_SEQ_STATUS seq_status;
    Variable *var;

    hash_seq_init(&seq_status, current_tab);
    // For every variable:
    while ((var = (Variable *)hash_seq_search(&seq_status)) != NULL) {

      VariableValue *vvar = &var->variable_value;

      while (vvar != NULL) {
        if (vvar->subxactid >= xact_id) {
          // If the value was set at this subxact ID or higher (at or after)
          if (vvar->previous == NULL) {
            // No previous values, the whole record is invalid
            bool found;
            hash_search(current_tab, NameStr(var->name), HASH_REMOVE, &found);
            break;
          } else {
            // Go up the chain
            vvar = vvar->previous;
          }
        } else {
          // If it was set at a lower subxact, use it
          if (vvar != &var->variable_value) {
            // Copy if it is not the top value.
            // We are not freeing up other values that may stand between the var and the value
            // as they will be freed with the transaction.
            memcpy(&var->variable_value, vvar, sizeof(var->variable_value));
          }
          break;
        }
      }
    }
  default:
    return;
  }
}

static bool transaction_callback_registered = false;

void transaction_callback(XactEvent event, void *arg) {
  switch (event) {
  case XACT_EVENT_ABORT:
  case XACT_EVENT_COMMIT:
  case XACT_EVENT_PARALLEL_ABORT:
  case XACT_EVENT_PARALLEL_COMMIT:
    if (subtransaction_callback_registered) {
      UnregisterSubXactCallback(subtransaction_callback, NULL);
      subtransaction_callback_registered = false;
    }
    if (transaction_callback_registered) {
#if PG_MAJORVERSION_NUM >= 16
      // Only unregister in Postgres >= 16 as per
      // https://github.com/postgres/postgres/commit/4d2a844242dcfb34e05dd0d880b1a283a514b16b In all
      // other versions, this callback will stay with its minimal (however, non-zero) cost.
      UnregisterXactCallback(transaction_callback, NULL);
      transaction_callback_registered = false;
#endif
    }
  default:
    // nothing to do
    return;
  }
}

Datum set(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("variable name must not be a null"));
  }
  Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
  if (value_type == InvalidOid) {
    ereport(ERROR, errmsg("value type can't be inferred"));
  }
  bool byval = get_typbyval(value_type);

  TransactionId txnid = GetTopTransactionId();
  SubTransactionId subtxnid = GetCurrentSubTransactionId();

  // Register callback to cleanup sub-transaction callback
  if (!transaction_callback_registered) {
    RegisterXactCallback(transaction_callback, NULL);
    transaction_callback_registered = true;
  }

  // Register callback to handle aborting sub-transactions
  // to restore values
  if (!subtransaction_callback_registered) {
    RegisterSubXactCallback(subtransaction_callback, NULL);
    subtransaction_callback_registered = true;
  }

  if (last_used_txnid != txnid) {
    // Initialize a new table. Old table will be deallocated with the transaction context
    const HASHCTL info = {
        .hcxt = TopTransactionContext, .keysize = NAMEDATALEN, .entrysize = sizeof(Variable)};
    current_tab = hash_create("omni_var variables", nvars, &info, TXN_HASH_ELEM | HASH_CONTEXT);
    last_used_txnid = txnid;
  }

  bool found;
  Variable *var = (Variable *)hash_search(current_tab, PG_GETARG_NAME(0), HASH_ENTER, &found);

  if (found && var->variable_value.subxactid < subtxnid) {
    // If the current sub-transaction is newer than the one used, copy the old value
    MemoryContext old_context = MemoryContextSwitchTo(TopTransactionContext);
    VariableValue *previous = (VariableValue *)palloc(sizeof(VariableValue));
    MemoryContextSwitchTo(old_context);
    memcpy(previous, &var->variable_value, sizeof(var->variable_value));
    var->variable_value.previous = previous;
  } else {
    // Otherwise, there are no older values
    var->variable_value.previous = NULL;
  }

  if (byval) {
    var->variable_value.value = PG_GETARG_DATUM(1);
  } else if (!PG_ARGISNULL(1)) {
    // Ensure we copy the value into the top transaction context
    // (matching the context of the hash table)
    // Otherwise it may be a shorter-lifetime context and then we may end
    // up with something unexpected here
    MemoryContext old_context = MemoryContextSwitchTo(TopTransactionContext);
    var->variable_value.value = PointerGetDatum(PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1)));
    MemoryContextSwitchTo(old_context);
  }
  var->variable_value.oid = value_type;
  var->variable_value.isnull = PG_ARGISNULL(1);
  var->variable_value.subxactid = subtxnid;

  if (PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  return var->variable_value.value;
}

Datum get(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("variable name must not be a null"));
  }
  Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
  if (value_type == InvalidOid) {
    ereport(ERROR, errmsg("default value type can't be inferred"));
  }
  TransactionId txnid = GetTopTransactionIdIfAny();
  if (txnid == InvalidTransactionId || last_used_txnid != txnid) {
    // No variables as we haven't set anything in the current session
    goto default_value;
  }

  bool found = false;
  Variable *var = (Variable *)hash_search(current_tab, PG_GETARG_NAME(0), HASH_ENTER, &found);
  if (found) {
    if (var->variable_value.isnull) {
      PG_RETURN_NULL();
    }
    if (var->variable_value.oid != value_type) {
      ereport(
          ERROR, errmsg("type mismatch"),
              errdetail("expected %s, got %s", format_type_be(var->variable_value.oid),
                        format_type_be(value_type)));
    }
    return var->variable_value.value;
  }
// If not found, return the default value
default_value:
  if (PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }
  return PG_GETARG_DATUM(1);
}
