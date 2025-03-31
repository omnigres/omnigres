/**
 * @file omni_sql.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <catalog/pg_type.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <string.h>
#include <utils/jsonb.h>
#include <utils/lsyscache.h>
#include <utils/syscache.h>

#include "nodes/pg_list.h"
#include "omni_sql.h"
#include "utils/elog.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(statement_in);

Datum statement_in(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement can't be NULL"));
  }
  char *statement = PG_GETARG_CSTRING(0);
  List *stmts = omni_sql_parse_statement(statement);
  char *deparsed = omni_sql_deparse_statement(stmts);
  text *deparsed_statement = cstring_to_text(deparsed);
  PG_RETURN_TEXT_P(deparsed_statement);
}

PG_FUNCTION_INFO_V1(statement_out);

Datum statement_out(PG_FUNCTION_ARGS) { PG_RETURN_CSTRING(text_to_cstring(PG_GETARG_TEXT_PP(0))); }

PG_FUNCTION_INFO_V1(add_cte);

Datum add_cte(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("Statement should not be NULL"));
  }
  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("CTE name should not be NULL"));
  }
  if (PG_ARGISNULL(2)) {
    ereport(ERROR, errmsg("CTE should not be NULL"));
  }
  if (PG_ARGISNULL(3)) {
    ereport(ERROR, errmsg("Recursive flag should not be NULL"));
  }

  if (PG_ARGISNULL(4)) {
    ereport(ERROR, errmsg("Prepend flag should not be NULL"));
  }

  text *statement = PG_GETARG_TEXT_PP(0);
  char *cstatement = text_to_cstring(statement);
  List *stmts = omni_sql_parse_statement(cstatement);

  text *cte_name = PG_GETARG_TEXT_PP(1);

  text *cte = PG_GETARG_TEXT_PP(2);
  char *ccte = text_to_cstring(cte);
  List *cte_stmts = omni_sql_parse_statement(ccte);

  bool recursive = PG_GETARG_BOOL(3);
  bool prepend = PG_GETARG_BOOL(4);

  stmts = omni_sql_add_cte(stmts, text_to_cstring(cte_name), cte_stmts, recursive, prepend);

  char *deparsed = omni_sql_deparse_statement(stmts);
  text *deparsed_statement = cstring_to_text(deparsed);

  PG_RETURN_TEXT_P(deparsed_statement);
}

PG_FUNCTION_INFO_V1(is_parameterized);

Datum is_parameterized(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement can't be NULL"));
  }
  text *statement = PG_GETARG_TEXT_PP(0);
  List *stmts = omni_sql_parse_statement(text_to_cstring(statement));

  PG_RETURN_BOOL(omni_sql_is_parameterized(stmts));
}

PG_FUNCTION_INFO_V1(is_valid);

Datum is_valid(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement can't be NULL"));
  }
  text *statement = PG_GETARG_TEXT_PP(0);
  char *string = text_to_cstring(statement);
  List *stmts = omni_sql_parse_statement(string);

  PG_RETURN_BOOL(omni_sql_is_valid(stmts, NULL));
}

PG_FUNCTION_INFO_V1(raw_statements);

static inline void find_line_col(char *str, int offset, int *line, int *col) {
  *line = 1;
  *col = 1;

  for (int i = 0; i < offset; i++) {
    if (str[i] == '\0') {
      return;
    }
    if (str[i] == '\n') {
      (*line)++;
      *col = 1;
    } else {
      (*col)++;
    }
  }
}

static inline int find_non_whitespace(char *str) {
  int offset = 0;

  while (str[offset]) {
    if (!isspace((unsigned char)str[offset])) {
      return offset;
    }
    offset++;
  }

  // If only whitespace, use it as is (but this shouldn't happen, there's no statement!)
  return 0;
}

Datum raw_statements(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statements can't be NULL"));
  }
  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("preserve_transactions flag can't be NULL"));
  }
  char *statement = PG_GETARG_CSTRING(0);
  bool preserve_transactions = PG_GETARG_BOOL(1);

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  List *stmts = omni_sql_parse_statement(statement);

  ListCell *lc;
  text *cmd;
  text *tx = NULL;
  int tx_line, tx_col;
  foreach (lc, stmts) {
    switch (nodeTag(lfirst(lc))) {
    case T_RawStmt: {
      RawStmt *raw_stmt = lfirst_node(RawStmt, lc);

      int line, col;
      int actual_start = find_non_whitespace(statement + raw_stmt->stmt_location);
      find_line_col(statement, raw_stmt->stmt_location + actual_start, &line, &col);
      cmd = raw_stmt->stmt_len == 0
                ? cstring_to_text(statement + raw_stmt->stmt_location + actual_start)
                : cstring_to_text_with_len(statement + raw_stmt->stmt_location + actual_start,
                                           raw_stmt->stmt_len - actual_start);

      if (preserve_transactions) {
        // normalize statement so we can differentiate between BEGIN and END
        char *deparsed = omni_sql_deparse_statement(list_make1(raw_stmt));
        bool is_end = strcmp(deparsed, "COMMIT") == 0 || strcmp(deparsed, "ROLLBACK") == 0;

        if (nodeTag(raw_stmt->stmt) == T_TransactionStmt && tx == NULL && !is_end) {
          // begin transaction
          tx = cmd;
          tx_line = line;
          tx_col = col;
        } else if (nodeTag(raw_stmt->stmt) == T_TransactionStmt && tx != NULL && !is_end) {
          // nested begin, probably not what the user wants
          ereport(ERROR, errmsg("nested transactions are not supported"));
        } else if (nodeTag(raw_stmt->stmt) == T_TransactionStmt && tx != NULL && is_end) {
          // end transaction, we have to concatenate the last command before clearing tx
          Datum values[3] = {DirectFunctionCall2(textcat,
                                                 DirectFunctionCall2(textcat, PointerGetDatum(tx),
                                                                     CStringGetTextDatum("; ")),
                                                 PointerGetDatum(cmd)),
                             Int32GetDatum(tx_line), Int32GetDatum(tx_col)};
          tx = NULL;
          bool isnull[3] = {false, false, false};
          tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
        } else if (tx != NULL) {
          // within transaction we just concatenate the command and proceed
          tx = DatumGetTextPP(DirectFunctionCall2(
              textcat, DirectFunctionCall2(textcat, PointerGetDatum(tx), CStringGetTextDatum("; ")),
              PointerGetDatum(cmd)));
        } else {
          // no transaction, output command immediately
          Datum values[3] = {PointerGetDatum(cmd), Int32GetDatum(line), Int32GetDatum(col)};
          bool isnull[3] = {false, false, false};
          tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
        }
      } else {
        Datum values[3] = {PointerGetDatum(cmd), Int32GetDatum(line), Int32GetDatum(col)};
        bool isnull[3] = {false, false, false};
        tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
      }
      break;
    }
    default:
      break;
    }
  };

  if (tx != NULL) {
    // handle cases of unfinished transactions
    ereport(ERROR, errmsg("unfinished transaction"));
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(statement_type);
Datum statement_type(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement should not be NULL"));
  }

  text *statement = PG_GETARG_TEXT_PP(0);
  char *cstatement = text_to_cstring(statement);
  List *stmts = omni_sql_parse_statement(cstatement);

  if (list_length(stmts) > 1) {
    PG_RETURN_CSTRING("MultiStmt");
  }

  char *val;

  ListCell *lc;
  foreach (lc, stmts) {
    switch (nodeTag(lfirst_node(RawStmt, lc)->stmt)) {
    case T_InsertStmt:
      val = "InsertStmt";
      break;
    case T_DeleteStmt:
      val = "DeleteStmt";
      break;
    case T_UpdateStmt:
      val = "UpdateStmt";
      break;
#if PG_MAJORVERSION_NUM > 14
    case T_MergeStmt:
      val = "MergeStmt";
      break;
#endif
    case T_SelectStmt:
      val = "SelectStmt";
      break;
    case T_SetOperationStmt:
      val = "SetOperationStmt";
      break;
#if PG_MAJORVERSION_NUM > 13
    case T_ReturnStmt:
      val = "ReturnStmt";
      break;
    case T_PLAssignStmt:
      val = "PLAssignStmt";
      break;
#endif
    case T_CreateSchemaStmt:
      val = "CreateSchemaStmt";
      break;
    case T_AlterTableStmt:
      val = "AlterTableStmt";
      break;
    case T_ReplicaIdentityStmt:
      val = "ReplicaIdentityStmt";
      break;
    case T_AlterCollationStmt:
      val = "AlterCollationStmt";
      break;
    case T_AlterDomainStmt:
      val = "AlterDomainStmt";
      break;
    case T_GrantStmt:
      val = "GrantStmt";
      break;
    case T_GrantRoleStmt:
      val = "GrantRoleStmt";
      break;
    case T_AlterDefaultPrivilegesStmt:
      val = "AlterDefaultPrivilegesStmt";
      break;
    case T_CopyStmt:
      val = "CopyStmt";
      break;
    case T_VariableSetStmt:
      val = "VariableSetStmt";
      break;
    case T_VariableShowStmt:
      val = "VariableShowStmt";
      break;
    case T_CreateStmt:
      val = "CreateStmt";
      break;
    case T_CreateTableSpaceStmt:
      val = "CreateTableSpaceStmt";
      break;
    case T_DropTableSpaceStmt:
      val = "DropTableSpaceStmt";
      break;
    case T_AlterTableSpaceOptionsStmt:
      val = "AlterTableSpaceOptionsStmt";
      break;
    case T_AlterTableMoveAllStmt:
      val = "AlterTableMoveAllStmt";
      break;
    case T_CreateExtensionStmt:
      val = "CreateExtensionStmt";
      break;
    case T_AlterExtensionStmt:
      val = "AlterExtensionStmt";
      break;
    case T_AlterExtensionContentsStmt:
      val = "AlterExtensionContentsStmt";
      break;
    case T_CreateFdwStmt:
      val = "CreateFdwStmt";
      break;
    case T_AlterFdwStmt:
      val = "AlterFdwStmt";
      break;
    case T_CreateForeignServerStmt:
      val = "CreateForeignServerStmt";
      break;
    case T_AlterForeignServerStmt:
      val = "AlterForeignServerStmt";
      break;
    case T_CreateForeignTableStmt:
      val = "CreateForeignTableStmt";
      break;
    case T_CreateUserMappingStmt:
      val = "CreateUserMappingStmt";
      break;
    case T_AlterUserMappingStmt:
      val = "AlterUserMappingStmt";
      break;
    case T_DropUserMappingStmt:
      val = "DropUserMappingStmt";
      break;
    case T_ImportForeignSchemaStmt:
      val = "ImportForeignSchemaStmt";
      break;
    case T_CreatePolicyStmt:
      val = "CreatePolicyStmt";
      break;
    case T_AlterPolicyStmt:
      val = "AlterPolicyStmt";
      break;
    case T_CreateAmStmt:
      val = "CreateAmStmt";
      break;
    case T_CreateTrigStmt:
      val = "CreateTrigStmt";
      break;
    case T_CreateEventTrigStmt:
      val = "CreateEventTrigStmt";
      break;
    case T_AlterEventTrigStmt:
      val = "AlterEventTrigStmt";
      break;
    case T_CreatePLangStmt:
      val = "CreatePLangStmt";
      break;
    case T_CreateRoleStmt:
      val = "CreateRoleStmt";
      break;
    case T_AlterRoleStmt:
      val = "AlterRoleStmt";
      break;
    case T_AlterRoleSetStmt:
      val = "AlterRoleSetStmt";
      break;
    case T_DropRoleStmt:
      val = "DropRoleStmt";
      break;
    case T_CreateSeqStmt:
      val = "CreateSeqStmt";
      break;
    case T_AlterSeqStmt:
      val = "AlterSeqStmt";
      break;
    case T_DefineStmt:
      val = "DefineStmt";
      break;
    case T_CreateDomainStmt:
      val = "CreateDomainStmt";
      break;
    case T_CreateOpClassStmt:
      val = "CreateOpClassStmt";
      break;
    case T_CreateOpFamilyStmt:
      val = "CreateOpFamilyStmt";
      break;
    case T_AlterOpFamilyStmt:
      val = "AlterOpFamilyStmt";
      break;
    case T_DropStmt:
      val = "DropStmt";
      break;
    case T_TruncateStmt:
      val = "TruncateStmt";
      break;
    case T_CommentStmt:
      val = "CommentStmt";
      break;
    case T_SecLabelStmt:
      val = "SecLabelStmt";
      break;
    case T_DeclareCursorStmt:
      val = "DeclareCursorStmt";
      break;
    case T_ClosePortalStmt:
      val = "ClosePortalStmt";
      break;
    case T_FetchStmt:
      val = "FetchStmt";
      break;
    case T_IndexStmt:
      val = "IndexStmt";
      break;
    case T_CreateStatsStmt:
      val = "CreateStatsStmt";
      break;
    case T_AlterStatsStmt:
      val = "AlterStatsStmt";
      break;
    case T_CreateFunctionStmt:
      val = "CreateFunctionStmt";
      break;
    case T_AlterFunctionStmt:
      val = "AlterFunctionStmt";
      break;
    case T_DoStmt:
      val = "DoStmt";
      break;
    case T_CallStmt:
      val = "CallStmt";
      break;
    case T_RenameStmt:
      val = "RenameStmt";
      break;
    case T_AlterObjectDependsStmt:
      val = "AlterObjectDependsStmt";
      break;
    case T_AlterObjectSchemaStmt:
      val = "AlterObjectSchemaStmt";
      break;
    case T_AlterOwnerStmt:
      val = "AlterOwnerStmt";
      break;
    case T_AlterOperatorStmt:
      val = "AlterOperatorStmt";
      break;
    case T_AlterTypeStmt:
      val = "AlterTypeStmt";
      break;
    case T_RuleStmt:
      val = "RuleStmt";
      break;
    case T_NotifyStmt:
      val = "NotifyStmt";
      break;
    case T_ListenStmt:
      val = "ListenStmt";
      break;
    case T_UnlistenStmt:
      val = "UnlistenStmt";
      break;
    case T_TransactionStmt:
      val = "TransactionStmt";
      break;
    case T_CompositeTypeStmt:
      val = "CompositeTypeStmt";
      break;
    case T_CreateEnumStmt:
      val = "CreateEnumStmt";
      break;
    case T_CreateRangeStmt:
      val = "CreateRangeStmt";
      break;
    case T_AlterEnumStmt:
      val = "AlterEnumStmt";
      break;
    case T_ViewStmt:
      val = "ViewStmt";
      break;
    case T_LoadStmt:
      val = "LoadStmt";
      break;
    case T_CreatedbStmt:
      val = "CreatedbStmt";
      break;
    case T_AlterDatabaseStmt:
      val = "AlterDatabaseStmt";
      break;
#if PG_MAJORVERSION_NUM > 14
    case T_AlterDatabaseRefreshCollStmt:
      val = "AlterDatabaseRefreshCollStmt";
      break;
#endif
    case T_AlterDatabaseSetStmt:
      val = "AlterDatabaseSetStmt";
      break;
    case T_DropdbStmt:
      val = "DropdbStmt";
      break;
    case T_AlterSystemStmt:
      val = "AlterSystemStmt";
      break;
    case T_ClusterStmt:
      val = "ClusterStmt";
      break;
    case T_VacuumStmt:
      val = "VacuumStmt";
      break;
    case T_ExplainStmt:
      val = "ExplainStmt";
      break;
    case T_CreateTableAsStmt:
      val = "CreateTableAsStmt";
      break;
    case T_RefreshMatViewStmt:
      val = "RefreshMatViewStmt";
      break;
    case T_CheckPointStmt:
      val = "CheckPointStmt";
      break;
    case T_DiscardStmt:
      val = "DiscardStmt";
      break;
    case T_LockStmt:
      val = "LockStmt";
      break;
    case T_ConstraintsSetStmt:
      val = "ConstraintsSetStmt";
      break;
    case T_ReindexStmt:
      val = "ReindexStmt";
      break;
    case T_CreateConversionStmt:
      val = "CreateConversionStmt";
      break;
    case T_CreateCastStmt:
      val = "CreateCastStmt";
      break;
    case T_CreateTransformStmt:
      val = "CreateTransformStmt";
      break;
    case T_PrepareStmt:
      val = "PrepareStmt";
      break;
    case T_ExecuteStmt:
      val = "ExecuteStmt";
      break;
    case T_DeallocateStmt:
      val = "DeallocateStmt";
      break;
    case T_DropOwnedStmt:
      val = "DropOwnedStmt";
      break;
    case T_ReassignOwnedStmt:
      val = "ReassignOwnedStmt";
      break;
    case T_AlterTSDictionaryStmt:
      val = "AlterTSDictionaryStmt";
      break;
    case T_AlterTSConfigurationStmt:
      val = "AlterTSConfigurationStmt";
      break;
    case T_CreatePublicationStmt:
      val = "CreatePublicationStmt";
      break;
    case T_AlterPublicationStmt:
      val = "AlterPublicationStmt";
      break;
    case T_CreateSubscriptionStmt:
      val = "CreateSubscriptionStmt";
      break;
    case T_AlterSubscriptionStmt:
      val = "AlterSubscriptionStmt";
      break;
    case T_DropSubscriptionStmt:
      val = "DropSubscriptionStmt";
      break;
    case T_PlannedStmt:
      val = "PlannedStmt";
      break;
    default:
      val = "UnknownStmt";
    }
    PG_RETURN_CSTRING(val);
  }

  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(is_returning_statement);

Datum is_returning_statement(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement should not be NULL"));
  }

  text *statement = PG_GETARG_TEXT_PP(0);
  char *cstatement = text_to_cstring(statement);
  List *stmts = omni_sql_parse_statement(cstatement);

  PG_RETURN_BOOL(omni_sql_is_returning_statement(stmts));
}

PG_FUNCTION_INFO_V1(is_replace_statement);

Datum is_replace_statement(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement should not be NULL"));
  }

  text *statement = PG_GETARG_TEXT_PP(0);
  char *cstatement = text_to_cstring(statement);
  List *stmts = omni_sql_parse_statement(cstatement);

  PG_RETURN_BOOL(omni_sql_is_replace_statement(stmts));
}

PG_FUNCTION_INFO_V1(execute_parameterized);

/**
 * Holds information between multiple calls to the SRF
 * execute_parameterized function. Most of the fields are used to deal
 * with parameters of a prepared statement.
 */
typedef struct ExecCtx {
  // the actual statement to be executed.
  char *stmt;
  // returning is important to see if a given statement returns tuples
  // to the caller. This controls the flow of the main function
  bool returning;
  // needed to finish the SRF properly (by calling SRF_RETURN_DONE)
  bool done;
  // used to fetch rows when the statement actually support such (see `returning`)
  Portal portal;
  // the number of parameters for a prepared statement
  int nargs;
  // the (optional) types of the parameters of the prepared statement
  Oid *types;
  // is any parameter null?
  char *nulls;
  // the arguments to replace the parameters of the prepared statement
  Datum *values;
} ExecCtx;

/**
 * Extract a prepared statement's parameter values (possibly NULL) and
 * their types from the parameters array.
 */
static inline void extract_information_json_parameters(ExecCtx *call_ctx, FuncCallContext *funcctx,
                                                       Jsonb *parameters) {
  if (!JB_ROOT_IS_ARRAY(parameters)) {
    ereport(ERROR, errmsg("parameters must be a JSON array"));
  }
  JsonbIterator *iter = JsonbIteratorInit(&parameters->root);
  JsonbValue val;
  JsonbIteratorToken tok;
  uint32 cardinality = JsonContainerSize(&parameters->root);
  call_ctx->types =
      (Oid *)MemoryContextAlloc(funcctx->multi_call_memory_ctx, sizeof(Oid) * cardinality);
  call_ctx->nulls =
      (char *)MemoryContextAlloc(funcctx->multi_call_memory_ctx, sizeof(bool) * cardinality);
  call_ctx->values =
      (Datum *)MemoryContextAlloc(funcctx->multi_call_memory_ctx, sizeof(Datum) * cardinality);
  call_ctx->nargs = cardinality;
  int i = 0;
  while ((tok = JsonbIteratorNext(&iter, &val, true)) != WJB_DONE) {
    if (tok == WJB_ELEM) {
      call_ctx->nulls[i] = (val.type == jbvNull) ? 'n' : ' ';
      switch (val.type) {
      case jbvBool:
        call_ctx->types[i] = BOOLOID;
        call_ctx->values[i] = val.val.boolean;
        break;
      case jbvNumeric:
        call_ctx->types[i] = NUMERICOID;
        call_ctx->values[i] = NumericGetDatum(val.val.numeric);
        break;
      case jbvNull:
        // Handle null as if it was text, because we'd just need to
        // cast it (explicitly or implicitly)
        call_ctx->types[i] = TEXTOID;
        call_ctx->values[i] = PointerGetDatum(NULL);
        break;
      case jbvString:
        call_ctx->types[i] = TEXTOID;
        call_ctx->values[i] =
            PointerGetDatum(cstring_to_text_with_len(val.val.string.val, val.val.string.len));
        break;
      default:
        ereport(ERROR, errmsg("unsupported parameter type at index %i", i));
      }
      i++;
    }
  }
}

/**
 * Use or override the prepared statement's argument types based on the
 * optional type array.
 */
static inline void extract_information_json_types(ExecCtx *call_ctx, ArrayType *types) {
  ArrayIterator iter = array_create_iterator(types, 0, NULL);
  int i = 0;
  Datum type_val;
  bool isnull;
  while (array_iterate(iter, &type_val, &isnull)) {
    if (isnull) {
      goto next;
    }
    Oid id = DatumGetObjectId(type_val);
    if (call_ctx->nulls[i] == 'n') {
      goto complete;
    }
    if (call_ctx->types[i] == TEXTOID && id != TEXTOID) {
      Oid typioparam;
      Oid input_func;

      getTypeInputInfo(id, &input_func, &typioparam);
      call_ctx->values[i] = OidInputFunctionCall(
          input_func, text_to_cstring(DatumGetTextPP(call_ctx->values[i])), typioparam, -1);
    }
    // Handle numeric type specialization
    if (call_ctx->types[i] == NUMERICOID) {
      switch (id) {
      case INT2OID:
        call_ctx->values[i] =
            Int16GetDatum(numeric_int4_opt_error(DatumGetNumeric(call_ctx->values[i]), NULL));
        break;
      case INT4OID:
        call_ctx->values[i] =
            Int32GetDatum(numeric_int4_opt_error(DatumGetNumeric(call_ctx->values[i]), NULL));
        break;
      case INT8OID:
#if PG_MAJORVERSION_NUM >= 17
        call_ctx->values[i] =
            Int64GetDatum(numeric_int8_opt_error(DatumGetNumeric(call_ctx->values[i]), NULL));
#else
        call_ctx->values[i] = DirectFunctionCall1(numeric_int8, call_ctx->values[i]);
#endif
        break;
      case FLOAT4OID:
        call_ctx->values[i] = DirectFunctionCall1(numeric_float4, call_ctx->values[i]);
        break;
      case FLOAT8OID:
        call_ctx->values[i] = DirectFunctionCall1(numeric_float8, call_ctx->values[i]);
        break;
      default: {
        HeapTuple typeTuple = SearchSysCache1(TYPEOID, ObjectIdGetDatum(id));
        if (!HeapTupleIsValid(typeTuple)) {
          ereport(ERROR, (errmsg("cache lookup failed for type %u", id)));
        }

        char *typename = pstrdup(NameStr(((Form_pg_type)GETSTRUCT(typeTuple))->typname));

        ReleaseSysCache(typeTuple);

        ereport(ERROR, errmsg("can't convert numeric to type %s", typename));
        break;
      }
      }
    }
  complete:
    call_ctx->types[i] = id;
  next:
    i++;
  }
  array_free_iterator(iter);
}

Datum execute_parameterized(PG_FUNCTION_ARGS) {
  FuncCallContext *funcctx;
  if (SRF_IS_FIRSTCALL()) {
    if (PG_ARGISNULL(0)) {
      ereport(ERROR, errmsg("statement should not be NULL"));
    }

    funcctx = SRF_FIRSTCALL_INIT();
    MemoryContext oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

    text *statement = PG_GETARG_TEXT_PP(0);
    char *cstatement = text_to_cstring(statement);

    ExecCtx *call_ctx = palloc(sizeof(*call_ctx));
    call_ctx->stmt = cstatement;
    call_ctx->done = false;
    call_ctx->portal = NULL;

    // Check parameters
    if (PG_ARGISNULL(1)) {
      call_ctx->nargs = 0;
    } else {
      extract_information_json_parameters(call_ctx, funcctx, PG_GETARG_JSONB_P(1));
      if (!PG_ARGISNULL(2)) {
        extract_information_json_types(call_ctx, PG_GETARG_ARRAYTYPE_P(2));
      }
    }
    SPI_connect();
    // Prepare the cursor
    SPIPlanPtr plan = SPI_prepare(call_ctx->stmt, call_ctx->nargs, call_ctx->types);
    // is this a plan that could return rows?
    call_ctx->returning = SPI_is_cursor_plan(plan);
    if (call_ctx->returning) {
      if (plan == NULL) {
        ereport(ERROR, errmsg("%s", SPI_result_code_string(SPI_result)));
      }
      call_ctx->portal =
          SPI_cursor_open("_omni_sql_execute", plan, call_ctx->values, call_ctx->nulls, false);
      // Fetch zero records just to get the tupdesc
      SPI_cursor_fetch(call_ctx->portal, true, 0);
      // Switch to multi-call memory instead of SPI
      MemoryContext oldcontext1 = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
      // Copy tupdesc
      funcctx->tuple_desc = CreateTupleDescCopyConstr(SPI_tuptable->tupdesc);
      BlessTupleDesc(funcctx->tuple_desc);
      funcctx->attinmeta = TupleDescGetAttInMetadata(funcctx->tuple_desc);
      MemoryContextSwitchTo(oldcontext1);
      SPI_finish();
    } else {
      SPI_finish();
      TupleDesc tupdesc = CreateTemplateTupleDesc(1);
      TupleDescInitEntry(tupdesc, (AttrNumber)1, "rows", INT8OID, -1, 0);
      BlessTupleDesc(tupdesc);

      funcctx->attinmeta = TupleDescGetAttInMetadata(tupdesc);
      funcctx->tuple_desc = tupdesc;
    }

    funcctx->user_fctx = call_ctx;

    MemoryContextSwitchTo(oldcontext);
  }

  funcctx = SRF_PERCALL_SETUP();
  ExecCtx *call_ctx = (ExecCtx *)funcctx->user_fctx;
  if (call_ctx->done) {
    SRF_RETURN_DONE(funcctx);
  }

  if (call_ctx->returning) {
    SPI_connect();
    SPI_cursor_fetch(call_ctx->portal, true, 1);
    switch (SPI_tuptable->numvals) {
    case 1: {
      MemoryContext o = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
      Datum result = heap_copy_tuple_as_datum(*SPI_tuptable->vals, funcctx->tuple_desc);
      MemoryContextSwitchTo(o);
      SPI_finish();
      SRF_RETURN_NEXT(funcctx, result);
    }
    case 0:
      SPI_cursor_close(call_ctx->portal);
      SPI_finish();
      SRF_RETURN_DONE(funcctx);
    default:
      SPI_finish();
      Assert(false);
    }
  } else {
    int64 processed = 0;

    SPI_connect();

    int rc = SPI_execute_with_args(call_ctx->stmt, call_ctx->nargs, call_ctx->types,
                                   call_ctx->values, call_ctx->nulls, false, 0);
    if (rc < 0) {
      ereport(ERROR, errmsg("%s", SPI_result_code_string(rc)));
    }
    processed = (int64)SPI_processed;
    SPI_finish();

    Datum values[1] = {Int64GetDatum(processed)};
    bool nulls[1] = {false};

    HeapTuple tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);
    Datum result = HeapTupleGetDatum(tuple);

    call_ctx->done = true;
    SRF_RETURN_NEXT(funcctx, result);
  }
}
