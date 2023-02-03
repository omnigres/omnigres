/**
 * @file omni_sql.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <funcapi.h>
#include <parser/parser.h>
#include <utils/builtins.h>

#include <nodes/nodeFuncs.h>

#include "deparse.h"

PG_MODULE_MAGIC;

static List *parse_statement(char *statement) {
#if PG_MAJORVERSION_NUM == 13
  List *stmts = raw_parser(statement);
#else
  List *stmts = raw_parser(statement, RAW_PARSE_DEFAULT);
#endif
  return stmts;
}

PG_FUNCTION_INFO_V1(statement_in);

Datum statement_in(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement can't be NULL"));
  }
  char *statement = PG_GETARG_CSTRING(0);
  List *stmts = parse_statement(statement);
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
  List *stmts = parse_statement(cstatement);

  if (list_length(stmts) != 1) {
    ereport(ERROR, errmsg("Statement should contain one and only one statement"));
  }

  text *cte_name = PG_GETARG_TEXT_PP(1);

  text *cte = PG_GETARG_TEXT_PP(2);
  char *ccte = text_to_cstring(cte);
  List *cte_stmts = parse_statement(ccte);

  if (list_length(cte_stmts) != 1) {
    ereport(ERROR, errmsg("CTE should contain one and only one statement"));
  }

  void *node = linitial(stmts);

  bool recursive = PG_GETARG_BOOL(3);
  bool prepend = PG_GETARG_BOOL(4);

  CommonTableExpr cte_node = {
    .type = T_CommonTableExpr,
    .ctename = text_to_cstring(cte_name),
    .aliascolnames = NULL,
    .ctematerialized = CTEMaterializeDefault,
    .ctequery = linitial_node(RawStmt, cte_stmts)->stmt,
#if PG_VERSION_MAJORNUM >= 14
    .search_clause = NULL,
    .cycle_clause = NULL,
#endif
    .location = -1, // unknown location
    .cterecursive = recursive,
    .cterefcount = 0,
    .ctecolnames = NULL,
    .ctecoltypes = NULL,
    .ctecoltypmods = NULL,
    .ctecolcollations = NULL
  };

  WithClause **with = NULL;
dispatch:
  switch (nodeTag(node)) {
  case T_RawStmt: {
    RawStmt *stmt = castNode(RawStmt, node);
    node = stmt->stmt;
    goto dispatch;
  }
  case T_SelectStmt: {
    SelectStmt *select = castNode(SelectStmt, node);
    with = &select->withClause;
    break;
  }
  case T_InsertStmt: {
    InsertStmt *insert = castNode(InsertStmt, node);
    with = &insert->withClause;
    break;
  }
  case T_UpdateStmt: {
    UpdateStmt *update = castNode(UpdateStmt, node);
    with = &update->withClause;
    break;
  }
  case T_DeleteStmt: {
    DeleteStmt *delete = castNode(DeleteStmt, node);
    with = &delete->withClause;
    break;
  }
  default:
    ereport(ERROR, errmsg("no supported statement found"));
  }

  if (*with == NULL) {
    WithClause *new_with = palloc(sizeof(*new_with));
    new_with->type = T_WithClause;
    new_with->location = -1;
    new_with->recursive = recursive;
    new_with->ctes = list_make1(&cte_node);
    *with = new_with;
  } else {
    List *ctes = (*with)->ctes;
    if (prepend) {
      ctes = list_insert_nth(ctes, 0, &cte_node);
    } else {
      ctes = lappend(ctes, &cte_node);
    }
  }
  char *deparsed = omni_sql_deparse_statement(stmts);
  text *deparsed_statement = cstring_to_text(deparsed);

  PG_RETURN_TEXT_P(deparsed_statement);
}

PG_FUNCTION_INFO_V1(is_parameterized);

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

Datum is_parameterized(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statement can't be NULL"));
  }
  text *statement = PG_GETARG_TEXT_PP(0);
  List *stmts = parse_statement(text_to_cstring(statement));

  ListCell *stmt;
  foreach (stmt, stmts) {
    bool contains = false;
    raw_expression_tree_walker(castNode(RawStmt, lfirst(stmt))->stmt, contains_param_walker,
                               &contains);
    if (contains) {
      PG_RETURN_BOOL(true);
    }
  }
  PG_RETURN_BOOL(false);
}