// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <catalog/pg_proc.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>
#include <utils/syscache.h>

#include <SWI-Prolog.h>

#include "predicates.h"

bool is_stub = false;

PG_MODULE_MAGIC;

static bool initialized = false;

static char *current_error = NULL;
static int current_error_level = 0;
static int current_error_code = 0;

foreign_t message_hook(term_t term, term_t kind, term_t lines) {
  term_t error = PL_new_term_ref();
  PL_put_atom(error, PL_new_atom("error"));
  if (PL_compare(error, kind) == 0) {
    term_t term_str = PL_new_term_refs(2);
    PL_put_term(term_str, term);
    if (!PL_call_predicate(NULL, PL_Q_NORMAL, PL_predicate("message_to_string", 2, NULL),
                           term_str)) {
      current_error = "failed calling message_to_string/2";
      current_error_level = ERROR;
      current_error_code = ERRCODE_INTERNAL_ERROR;
    } else {
      char *msg;
      size_t errlen;
      PL_get_string(term_str + 1, &msg, &errlen);
      current_error = pstrdup(msg);
      current_error_level = ERROR;
      current_error_code = ERRCODE_EXTERNAL_ROUTINE_EXCEPTION;
    }
  }
  return TRUE;
}

static int term_t_to_datum(term_t term, Datum *datum, Oid *oid, bool params) {
  Datum result;
  switch (PL_term_type(term)) {
  case PL_INTEGER: {
    union {
      int i;
      int64_t i64;
    } val;
    if (PL_get_integer(term, &val.i)) {
      if (datum != NULL)
        *datum = Int32GetDatum(val.i);
      if (oid != NULL)
        *oid = INT4OID;
    } else if (PL_get_int64(term, &val.i64)) {
      if (datum != NULL)
        *datum = Int64GetDatum(val.i);
      if (oid != NULL)
        *oid = INT8OID;
    } else {
      return FALSE;
    }
    break;
  }
  case PL_STRING: {
    char *string;
    size_t string_length;
    if (!PL_get_string_chars(term, &string, &string_length)) {
      return FALSE;
    }
    if (params) {
      char *managed_string = pstrdup(string);
      if (datum != NULL)
        *datum = CStringGetDatum(managed_string);
      if (oid != NULL)
        *oid = CSTRINGOID;
    } else {
      if (datum != NULL)
        *datum = PointerGetDatum(cstring_to_text_with_len(string, string_length));
      if (oid != NULL)
        *oid = TEXTOID;
    }
    break;
  }
  case PL_ATOM: {
    char *string;
    size_t string_length;
    PL_get_atom_nchars(term, &string_length, &string);
    if (params) {
      char *managed_string = pstrdup(string);
      if (datum != NULL)
        *datum = CStringGetDatum(managed_string);
      if (oid != NULL)
        *oid = CSTRINGOID;
    } else {
      if (datum != NULL)
        *datum = PointerGetDatum(cstring_to_text_with_len(string, string_length));
      if (oid != NULL)
        *oid = TEXTOID;
    }
    break;
  }
  default:
    // TODO: cover the rest of types where possible
    return FALSE;
  }
  return TRUE;
}

static oidvector *current_argtypes;
static char **current_argnames;
static FunctionCallInfo current_call;

foreign_t arg(term_t argument, term_t value) {
  int argn = 0;
  char *argname = NULL;

  // Try to find out the argument (an index or an atom name)
  if (!PL_get_integer(argument, &argn) && !PL_get_atom_chars(argument, &argname)) {
    return FALSE;
  }

  // If it is an atom, get argument's index
  if (current_argnames != NULL && argname != NULL) {
    int index = 0;
    while (current_argnames[index] != NULL) {
      if (strcmp(current_argnames[index], argname) == 0) {
        // We have a match
        argn = index + 1;
        break;
      }
      index++;
    }
  }

  if (argn > current_call->nargs) {
    return FALSE;
  }

  Oid oid = current_argtypes->values[argn - 1];
  Datum datum = current_call->args[argn - 1].value;
  switch (oid) {
  case INT2OID:
    return PL_unify_integer(value, DatumGetInt16(datum));
  case INT4OID:
    return PL_unify_integer(value, DatumGetInt32(datum));
  case INT8OID:
    return PL_unify_int64(value, DatumGetInt64(datum));
  case TEXTOID: {
    text *txt = DatumGetTextPP(datum);
    return PL_unify_string_nchars(value, VARSIZE_ANY_EXHDR(txt), VARDATA_ANY(txt));
  }
  default:
    return FALSE;
  }
}

struct query_ctx {
  int current_pos;
  Portal portal;
};

foreign_t query(term_t query, term_t args, term_t out, control_t handle) {
  struct query_ctx *ctx;
  char *query_str;
  switch (PL_foreign_control(handle)) {
  case PL_FIRST_CALL: {
    size_t l;
    if (!PL_get_string(query, &query_str, &l)) {
      return FALSE;
    }
    term_t head = PL_new_term_ref();
    term_t tail = PL_new_term_ref();
    term_t list = args;
    int list_len = 0;

    // Count the length of arguments
    while (PL_get_list(list, head, tail)) {
      list_len++;
      list = tail;
    }

    list = args;

    ParamListInfo params = makeParamList(list_len);
    int i = 0;
    while (PL_get_list(list, head, tail)) {

      params->params[i] = (ParamExternData){.pflags = 0, .isnull = false};

      if (!PL_is_ground(head)) {
        return FALSE;
      }

      if (!term_t_to_datum(head, &params->params[i].value, &params->params[i].ptype, true)) {
        return FALSE;
      }

      i++;
      list = tail;
    }

    ctx = (struct query_ctx *)palloc0(sizeof(*ctx));
    SPIParseOpenOptions opts = {
        .read_only = false, .cursorOptions = CURSOR_OPT_NO_SCROLL, .params = params};
    ctx->portal = SPI_cursor_parse_open(NULL, query_str, &opts);
    ctx->current_pos = -1;
    break;
  }
  case PL_REDO:
    ctx = (struct query_ctx *)PL_foreign_context(handle);
    break;
  case PL_PRUNED:
    ctx = (struct query_ctx *)PL_foreign_context(handle);
    goto complete;
  }
  ctx->current_pos++;

  SPI_cursor_fetch(ctx->portal, true, 1);
  if (SPI_processed == 0) {
    // No more rows, we're done
    goto complete;
  }

  HeapTuple data = SPI_tuptable->vals[0];
  // Number of columns
  int ncol = SPI_tuptable->tupdesc->natts;

  term_t head = PL_new_term_ref();
  term_t tail = PL_new_term_ref();

  while (PL_get_list(out, head, tail)) {

    if (PL_is_callable(head)) {

      atom_t name;
      size_t arity;
      PL_get_name_arity(head, &name, &arity);

      if (PL_new_atom("rownum") == name && arity == 1) {
        term_t arg = PL_new_term_ref();
        PL_get_arg(1, head, arg);
        if (!PL_unify_integer(arg, ctx->current_pos + 1)) {
          return FALSE;
        }
      }

      if (PL_new_atom("columns") == name) {
        // Iterate over arguments
        for (int i = 1; i <= arity; i++) {
          // For each argument
          term_t arg = PL_new_term_ref();
          if (PL_get_arg(i, head, arg)) {
            functor_t functor;
            if (PL_get_functor(arg, &functor)) {
              int arity = PL_functor_arity(functor);
              if (arity != 2) {
                return FALSE;
              }
              // Column to fetch
              term_t column = PL_new_term_ref();
              if (!PL_get_arg(1, arg, column)) {
                return FALSE;
              }
              // Column must be a number of an atom
              bool is_atom;
              int index = 0;
              if (!(is_atom = PL_is_atom(column)) && !PL_get_integer(column, &index)) {
                return FALSE;
              }
              // If it is an atom, we need to find the index
              if (is_atom) {
                char *expected_name;
                if (!PL_get_atom_chars(column, &expected_name)) {
                  return FALSE;
                }
                for (int i = 0; i < ncol; i++) {
                  if (strcmp(NameStr(SPI_tuptable->tupdesc->attrs[i].attname), expected_name) ==
                      0) {
                    // We found it
                    index = i + 1;
                    break;
                  }
                }
              }
              // If the index is outside of the scope, bail
              if (index < 1 || index > ncol) {
                return FALSE;
              }
              bool isnull;
              Oid oid = TupleDescAttr(SPI_tuptable->tupdesc, index - 1)->atttypid;
              Datum datum = SPI_getbinval(data, SPI_tuptable->tupdesc, index, &isnull);
              term_t val = PL_new_term_ref();
              switch (oid) {
              case INT2OID:
                PL_put_integer(val, DatumGetInt16(datum));
                break;
              case INT4OID:
                PL_put_integer(val, DatumGetInt32(datum));
                break;
              case INT8OID:
                PL_put_int64(val, DatumGetInt64(datum));
                break;
              case TEXTOID: {
                text *t = DatumGetTextPP(datum);
                PL_put_string_nchars(val, VARSIZE_ANY_EXHDR(t), VARDATA_ANY(t));
                break;
              }
              case CSTRINGOID: {
                PL_put_string_chars(val, DatumGetCString(datum));
                break;
              }
              default:
                PL_put_atom_chars(val, "unsupported");
              }
              // Get the right-hand side of the column=... term
              term_t rhs = PL_new_term_ref();
              if (!PL_get_arg(2, arg, rhs)) {
                return FALSE;
              }
              // Ready to unify
              if (!PL_unify_term(val, PL_TERM, rhs)) {
                return FALSE;
              }
            }
          }
        }
      }
    }
    out = tail;
  }

  // We don't know if there is any more data, so retry anyway
  PL_retry_address(ctx);

complete:
  SPI_cursor_close(ctx->portal);
  pfree(ctx);
  return FALSE;
}

PL_engine_t old_engine;
PL_engine_t current_engine;

static void report_error_if_any() {
  if (current_error != NULL) {
    char *err = current_error;
    current_error = NULL;

    PL_set_engine(old_engine, NULL);
    PL_destroy_engine(current_engine);

    ereport(current_error_level, errcode(current_error_code), errmsg("%s", err));
  }
}

Datum plprologX_call_handler(PG_FUNCTION_ARGS, bool sandbox) {
  Datum ret;
  bool isnull;

  /* Fetch the function's pg_proc entry. */
  HeapTuple pl_tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(fcinfo->flinfo->fn_oid));
  if (!HeapTupleIsValid(pl_tuple))
    elog(ERROR, "cache lookup failed for function %u", fcinfo->flinfo->fn_oid);

  /*
   * Extract and print the source text of the function.  This can be used as
   * a base for the function validation and execution.
   */
  Form_pg_proc pl_struct = (Form_pg_proc)GETSTRUCT(pl_tuple);
  char *proname = pstrdup(NameStr(pl_struct->proname));
  ret = SysCacheGetAttr(PROCOID, pl_tuple, Anum_pg_proc_prosrc, &isnull);
  if (isnull)
    elog(ERROR, "could not find source text of function \"%s\"", proname);
  char *source = DatumGetCString(DirectFunctionCall1(textout, ret));

  ret = SysCacheGetAttr(PROCOID, pl_tuple, Anum_pg_proc_proargtypes, &isnull);
  current_argtypes = (oidvector *)DatumGetPointer(ret);

  Datum proargnames = SysCacheGetAttr(PROCOID, pl_tuple, Anum_pg_proc_proargnames, &isnull);
  Datum proargmodes = SysCacheGetAttr(PROCOID, pl_tuple, Anum_pg_proc_proargmodes, &isnull);
  get_func_input_arg_names(proargnames, proargmodes, &current_argnames);

  ReleaseSysCache(pl_tuple);

  if (!initialized) {

    SPI_connect();

    SPI_execute_with_args("select probin from pg_proc where proname = $1", 1, (Oid[1]){TEXTOID},
                          (Datum[1]){PointerGetDatum(cstring_to_text("plprologu_call_handler"))},
                          (char[1]){' '}, false, 1);
    char *path = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);
    SPI_finish();

    PL_initialise(1, (char *[1]){path});
    PL_set_prolog_flag("debug", PL_BOOL, false);
    PL_set_prolog_flag("debug_on_error", PL_BOOL, false);
    PL_set_prolog_flag("debug_on_interrupt", PL_BOOL, false);

    install();

    initialized = true;
  }

  current_call = fcinfo;

  term_t file = PL_new_term_ref();
  PL_put_atom_chars(file, proname);

  SPI_connect();

  PL_thread_attr_t engine_attr = {
      .cancel = NULL,
      .flags = 0,
      .alias = 0,
      .max_queue_size = 0,
      .stack_limit = 0,
      .table_space = 0,
  };
  current_engine = PL_create_engine(&engine_attr);
  PL_set_engine(current_engine, &old_engine);

  // load_files
  predicate_t pred =
      PL_predicate(sandbox ? "$omni_sandbox_load_code" : "$omni_load_code", 2, "user");
  term_t params = PL_new_term_refs(2);
  PL_put_atom_chars(params, proname);
  PL_put_string_chars(params + 1, source);

  // Consult the file
  if (!PL_call_predicate(NULL, PL_Q_NORMAL, pred, params)) {
    printf("Failed to consult file\n");
  }

  report_error_if_any();

  Datum result = (Datum)0;

  term_t result_term = PL_new_term_ref();
  if (!PL_call_predicate(NULL, PL_Q_NORMAL,
                         PL_predicate(sandbox ? "$omni_sandbox_result" : "result", 1, NULL),
                         result_term)) {
    // return null if there are no results
    fcinfo->isnull = true;
  }

  report_error_if_any();

  Oid result_oid = InvalidOid;

  if (!term_t_to_datum(result_term, &result, &result_oid, false)) {
    ereport(WARNING, errmsg("unsupported result type %s, returning null",
                            result_oid == InvalidOid ? "Invalid" : format_type_be(result_oid)));
    fcinfo->isnull = true;
    result = 0;
  }

  SPI_finish();

  PL_set_engine(old_engine, NULL);
  PL_destroy_engine(current_engine);

  Oid result_type_oid = get_func_rettype(fcinfo->flinfo->fn_oid);
  if (result_oid != result_type_oid && result_oid != InvalidOid) {
    if ((result_oid == INT4OID && result_type_oid == INT2OID) ||
        (result_oid == INT4OID && result_type_oid == INT8OID)) {
      // Allow this implicit conversion
    } else {
      ereport(ERROR, errmsg("result type mismatch, expected %s, got %s",
                            format_type_be(result_type_oid), format_type_be(result_oid)));
    }
  }

  return result;
}

PG_FUNCTION_INFO_V1(plprologu_call_handler);

Datum plprologu_call_handler(PG_FUNCTION_ARGS) { return plprologX_call_handler(fcinfo, false); }

PG_FUNCTION_INFO_V1(plprolog_call_handler);

Datum plprolog_call_handler(PG_FUNCTION_ARGS) { return plprologX_call_handler(fcinfo, true); }