/**
 * @file omni_txn.c
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

PG_FUNCTION_INFO_V1(set_variable);
PG_FUNCTION_INFO_V1(get_variable);

static TransactionId last_used_txnid = InvalidTransactionId;
static HTAB *current_tab = NULL;

typedef struct {
  NameData name;
  bool isnull;
  Oid oid;
  Datum value;
} Variable;

static int nvars = 0;

void _PG_init() {
  DefineCustomIntVariable(
      "omni_txn.estimated_initial_variables_count", "Estimated number of transaction variables",
      "Estimated number of transaction variables, changes apply to the next transaction", &nvars,
      1024, 0, 65535, PGC_USERSET, 0, NULL, NULL, NULL);
}

#if PG_MAJORVERSION_NUM < 14
#define TXN_HASH_ELEM HASH_ELEM
#else
#define TXN_HASH_ELEM HASH_ELEM | HASH_STRINGS
#endif

Datum set_variable(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("variable name must not be a null"));
  }
  Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
  bool byval = get_typbyval(value_type);
  TransactionId txnid = GetCurrentTransactionId();
  if (last_used_txnid != txnid) {
    // Initialize a new table. Old table will be deallocated with the transaction context
    const HASHCTL info = {
        .hcxt = TopTransactionContext, .keysize = NAMEDATALEN, .entrysize = sizeof(Variable)};
    current_tab = hash_create("omni_txn variables", nvars, &info, TXN_HASH_ELEM | HASH_CONTEXT);
    last_used_txnid = txnid;
  }

  bool found;
  Variable *var = (Variable *)hash_search(current_tab, PG_GETARG_NAME(0), HASH_ENTER, &found);
  if (byval) {
    var->value = PG_GETARG_DATUM(1);
  } else {
    // Ensure we copy the value into the top transaction context
    // (matching the context of the hash table)
    // Otherwise it may be a shorter-lifetime context and then we may end
    // up with something unexpected here
    MemoryContext old_context = CurrentMemoryContext;
    MemoryContextSwitchTo(TopTransactionContext);
    var->value = PointerGetDatum(PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1)));
    MemoryContextSwitchTo(old_context);
  }
  var->oid = value_type;
  var->isnull = PG_ARGISNULL(1);

  if (PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  return var->value;
}

Datum get_variable(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("variable name must not be a null"));
  }
  Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
  TransactionId txnid = GetCurrentTransactionIdIfAny();
  if (txnid == InvalidTransactionId || last_used_txnid != txnid) {
    // No variables as we haven't set anything in the current session
    goto default_value;
  }

  bool found = false;
  Variable *var = (Variable *)hash_search(current_tab, PG_GETARG_NAME(0), HASH_ENTER, &found);
  if (found) {
    if (var->isnull) {
      PG_RETURN_NULL();
    }
    if (var->oid != value_type) {
      ereport(
          ERROR, errmsg("type mismatch"),
          errdetail("expected %s, got %s", format_type_be(var->oid), format_type_be(value_type)));
    }
    return var->value;
  }
  // If not found, return the default value
default_value:
  if (PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }
  return PG_GETARG_DATUM(1);
}
