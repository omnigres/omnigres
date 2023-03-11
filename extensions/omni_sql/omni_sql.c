/**
 * @file omni_sql.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

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