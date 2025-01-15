#ifndef OMNI_SQL_H
#define OMNI_SQL_H
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <funcapi.h>
#include <parser/parser.h>
#include <utils/builtins.h>

#include <nodes/nodeFuncs.h>

#include "deparse.h"

List *omni_sql_parse_statement(char *statement);

bool omni_sql_get_with_clause(Node *node, WithClause ***with);

List *omni_sql_add_cte(List *stmts, char *cte_name, List *cte_stmts, bool recursive, bool prepend);

bool omni_sql_is_parameterized(List *stmts);

bool omni_sql_is_valid(List *stmts, char **error);

bool omni_sql_is_returning_statement(List *stmts);

bool omni_sql_is_replace_statement(List *stmts);

#endif // OMNI_SQL_H
