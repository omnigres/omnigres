/**
 * @file lib.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <funcapi.h>
#include <parser/analyze.h>
#include <parser/parser.h>
#include <stdbool.h>
#include <utils/builtins.h>

#include <nodes/nodeFuncs.h>

#include "deparse.h"
#include "nodes/nodes.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "utils/elog.h"

List *omni_sql_parse_statement(char *statement) {
#if PG_MAJORVERSION_NUM == 13
  List *stmts = raw_parser(statement);
#else
  List *stmts = raw_parser(statement, RAW_PARSE_DEFAULT);
#endif
  return stmts;
}

bool omni_sql_get_with_clause(Node *node, WithClause ***with) {
  Node *curNode = node;
dispatch:
  switch (nodeTag(curNode)) {
  case T_RawStmt: {
    RawStmt *stmt = castNode(RawStmt, curNode);
    curNode = stmt->stmt;
    goto dispatch;
  }
  case T_SelectStmt: {
    SelectStmt *select = castNode(SelectStmt, curNode);
    *with = &select->withClause;
    break;
  }
  case T_InsertStmt: {
    InsertStmt *insert = castNode(InsertStmt, curNode);
    *with = &insert->withClause;
    break;
  }
  case T_UpdateStmt: {
    UpdateStmt *update = castNode(UpdateStmt, curNode);
    *with = &update->withClause;
    break;
  }
  case T_DeleteStmt: {
    DeleteStmt *delete = castNode(DeleteStmt, curNode);
    *with = &delete->withClause;
    break;
  }
#if PG_MAJORVERSION_NUM >= 15
  case T_MergeStmt: {
    MergeStmt *delete = castNode(MergeStmt, curNode);
    *with = &delete->withClause;
    break;
  }
#endif
  default:
    return false;
  }
  return true;
}

List *omni_sql_add_cte(List *stmts, char *cte_name, List *cte_stmts, bool recursive, bool prepend) {

  if (list_length(stmts) != 1) {
    ereport(ERROR, errmsg("Statement should contain one and only one statement"));
  }

  if (list_length(cte_stmts) != 1) {
    ereport(ERROR, errmsg("CTE should contain one and only one statement"));
  }

  void *node = linitial(stmts);

  CommonTableExpr *cte_node = makeNode(CommonTableExpr);
  cte_node->ctename = cte_name;
  cte_node->ctematerialized = CTEMaterializeDefault;
  cte_node->ctequery = linitial_node(RawStmt, cte_stmts)->stmt;
  cte_node->cterecursive = recursive;

  WithClause **with;
  if (!omni_sql_get_with_clause(node, &with)) {
    ereport(ERROR, errmsg("no supported statement found"));
  }

  if (*with == NULL) {
    WithClause *new_with = makeNode(WithClause);
    new_with->location = -1;
    new_with->recursive = recursive;
    new_with->ctes = list_make1(cte_node);
    *with = new_with;
  } else {
    List *ctes = (*with)->ctes;
    if (prepend) {
      ctes = list_insert_nth(ctes, 0, cte_node);
    } else {
      ctes = lappend(ctes, cte_node);
    }
  }

  return stmts;
}

static bool contains_param_walker(Node *node, bool *contains) {
  if (node != NULL) {
    if (nodeTag(node) == T_ParamRef) {
      *contains = true;
      return true;
    } else {
      return raw_expression_tree_walker(node, contains_param_walker, contains);
    }
  }
  return false;
}

bool omni_sql_is_parameterized(List *stmts) {

  ListCell *stmt;
  foreach (stmt, stmts) {
    bool contains = false;
    raw_expression_tree_walker(castNode(RawStmt, lfirst(stmt))->stmt, contains_param_walker,
                               &contains);
    if (contains) {
      return true;
    }
  }
  return false;
}

bool omni_sql_is_valid(List *stmts, char **error) {
  if (omni_sql_is_parameterized(stmts)) {
    return false;
  }

  ListCell *lc = NULL;
  bool valid = true;
  MemoryContext memory_context = CurrentMemoryContext;
  // Don't call this hook during `parse_analyze_fixedparams`/`parse_analyze_varparams`
  // We currently presume this to be non-critical to deciding on the validity of the query.
  // Furthermore, currently, leaving this hook running makes `pg_stat_statements`'s hook
  // fail an assertion.
  post_parse_analyze_hook_type hook = post_parse_analyze_hook;
  post_parse_analyze_hook = NULL;

  foreach (lc, stmts) {
    RawStmt *stmt = lfirst_node(RawStmt, lc);
    PG_TRY();
    {
#if PG_MAJORVERSION_NUM >= 15
      parse_analyze_fixedparams(stmt, omni_sql_deparse_statement(list_make1(stmt)), NULL, 0, NULL);
#else
      int numparams = 0;
      parse_analyze_varparams(stmt, omni_sql_deparse_statement(list_make1(stmt)), NULL, &numparams);
      if (numparams != 0) {
        if (error != NULL) {
          *error = pstrdup("can't be parameterized");
        }
        goto done;
      }
#endif
    }
    PG_CATCH();
    {
      valid = false;
      if (error != NULL) {
        MemoryContextSwitchTo(memory_context);
        ErrorData *err = CopyErrorData();
        *error = err->message;
      }
      FlushErrorState();
      goto done;
    }
    PG_END_TRY();
  }
done:
  post_parse_analyze_hook = hook;
  return valid;
}

/*
 * Is the statement capable of returning rows? Similar to
 * https://www.postgresql.org/docs/current/spi-spi-is-cursor-plan.html
 */
bool omni_sql_is_returning_statement(List *stmts) {

  int len = list_length(stmts);
  if (len == 0) {
    return false;
  }

  Node *stmt = (len == 1 ? (linitial_node(RawStmt, stmts)) : (llast_node(RawStmt, stmts)))->stmt;
#if PG_MAJORVERSION_NUM < 18
#define returningList(type, stmt) castNode(type, stmt)->returningList
#else
#define returningList(type, stmt)                                                                  \
  (castNode(type, stmt)->returningClause ? castNode(type, stmt)->returningClause->exprs : NIL)
#endif
  switch (nodeTag(stmt)) {
  case T_SelectStmt:
    return true;
  case T_UpdateStmt:
    return list_length(returningList(UpdateStmt, stmt)) > 0;
  case T_InsertStmt:
    return list_length(returningList(InsertStmt, stmt)) > 0;
  case T_DeleteStmt:
    return list_length(returningList(DeleteStmt, stmt)) > 0;
#undef returningList
  default:
    return false;
  }
}

/*
 * Is the statement capable of replacing the object being defined?
 * When given multiple statements returns true only when all of them have the OR REPLACE clause.
 */
bool omni_sql_is_replace_statement(List *stmts) {
  bool all_stmts_replace = true;
  int len = list_length(stmts);
  if (len == 0) {
    return false;
  }

  ListCell *lc = NULL;
  foreach (lc, stmts) {
    Node *stmt = castNode(RawStmt, lfirst(lc))->stmt;
    switch (nodeTag(stmt)) {
    // we cast on each branch for the sake of type-safety
    case T_CreateTransformStmt:
      all_stmts_replace = all_stmts_replace && ((const CreateTransformStmt *)(stmt))->replace;
      break;
    case T_CreatePLangStmt:
      all_stmts_replace = all_stmts_replace && ((const CreatePLangStmt *)(stmt))->replace;
      break;
    case T_CreateFunctionStmt:
      all_stmts_replace = all_stmts_replace && ((const CreateFunctionStmt *)(stmt))->replace;
      break;
#if PG_MAJORVERSION_NUM > 13
    case T_CreateTrigStmt:
      all_stmts_replace = all_stmts_replace && ((const CreateTrigStmt *)(stmt))->replace;
      break;
#endif
    case T_RuleStmt:
      all_stmts_replace = all_stmts_replace && ((const RuleStmt *)(stmt))->replace;
      break;
    case T_ViewStmt:
      all_stmts_replace = all_stmts_replace && ((const ViewStmt *)(stmt))->replace;
      break;
    case T_DefineStmt:
      all_stmts_replace = all_stmts_replace && ((const DefineStmt *)(stmt))->replace;
      break;
    default:
      all_stmts_replace = all_stmts_replace && false;
    }
  }
  return all_stmts_replace;
}
