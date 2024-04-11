/**
 * @file omni_sql.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <miscadmin.h>

#include "omni_sql.h"

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

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  char *statement = PG_GETARG_CSTRING(0);

  List *stmts = omni_sql_parse_statement(statement);

  ListCell *lc;
  foreach (lc, stmts) {
    switch (nodeTag(lfirst(lc))) {
    case T_RawStmt: {
      RawStmt *raw_stmt = lfirst_node(RawStmt, lc);
      int line, col;
      int actual_start = find_non_whitespace(statement + raw_stmt->stmt_location);
      find_line_col(statement, raw_stmt->stmt_location + actual_start, &line, &col);
      Datum values[3] = {
          PointerGetDatum(
              raw_stmt->stmt_len == 0
                  ? cstring_to_text(statement + raw_stmt->stmt_location + actual_start)
                  : cstring_to_text_with_len(statement + raw_stmt->stmt_location + actual_start,
                                             raw_stmt->stmt_len)),
          Int32GetDatum(line), Int32GetDatum(col)};
      bool isnull[3] = {false, false, false};
      tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
      break;
    }
    default:
      break;
    }
  };

  tuplestore_donestoring(tupstore);

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
