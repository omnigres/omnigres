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

#include "omni_var.h"

static HTAB *session_tab = NULL;

PG_FUNCTION_INFO_V1(set_session);
PG_FUNCTION_INFO_V1(get_session);

Datum set_session(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("variable name must not be a null"));
  }
  Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
  if (value_type == InvalidOid) {
    ereport(ERROR, errmsg("value type can't be inferred"));
  }
  bool byval = get_typbyval(value_type);
  int16 typlen = get_typlen(value_type);

  if (session_tab == NULL) {
    const HASHCTL info = {
        .hcxt = TopMemoryContext, .keysize = NAMEDATALEN, .entrysize = sizeof(Variable)};
    session_tab = hash_create("omni_var session variables", num_scope_vars, &info,
                              VAR_HASH_ELEM | HASH_CONTEXT);
  }

  bool found;
  Variable *var = (Variable *)hash_search(session_tab, PG_GETARG_NAME(0), HASH_ENTER, &found);

  VariableValue *vvar = var->variable_value;
  if (found) {
    MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
    vvar = (VariableValue *)palloc(sizeof(VariableValue));
    MemoryContextSwitchTo(old_context);
    // Ensure variable points to it
    var->variable_value = vvar;
  } else {
    if (!found) {
      // Important: initialize the point to the initial allocation
      vvar = var->variable_value = &var->initial_allocation;
    }
    // Otherwise, there are no older values
    vvar->previous = NULL;
  }

  if (byval) {
    vvar->value = PG_GETARG_DATUM(1);
  } else if (!PG_ARGISNULL(1)) {
    // Ensure we copy the value into the top memory context
    // (matching the context of the hash table)
    // Otherwise it may be a shorter-lifetime context and then we may end
    // up with something unexpected here
    MemoryContext old_context = MemoryContextSwitchTo(TopMemoryContext);
    if (typlen == -1) {
      vvar->value = PointerGetDatum(PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(1)));
    } else {
      void *ptr = palloc(typlen);
      memcpy(ptr, DatumGetPointer(PG_GETARG_DATUM(1)), typlen);
      vvar->value = PointerGetDatum(ptr);
    }
    MemoryContextSwitchTo(old_context);
  }
  vvar->oid = value_type;
  vvar->isnull = PG_ARGISNULL(1);

  if (PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  return vvar->value;
}

Datum get_session(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("variable name must not be a null"));
  }
  Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
  if (value_type == InvalidOid) {
    ereport(ERROR, errmsg("default value type can't be inferred"));
  }

  if (session_tab == NULL) {
    goto default_value;
  }

  bool found = false;
  Variable *var = (Variable *)hash_search(session_tab, PG_GETARG_NAME(0), HASH_FIND, &found);
  if (found) {
    VariableValue *vvar = var->variable_value;
    if (vvar->isnull) {
      PG_RETURN_NULL();
    }
    if (vvar->oid != value_type) {
      ereport(
          ERROR, errmsg("type mismatch"),
          errdetail("expected %s, got %s", format_type_be(vvar->oid), format_type_be(value_type)));
    }
    return vvar->value;
  }
// If not found, return the default value
default_value:
  if (PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }
  return PG_GETARG_DATUM(1);
}
