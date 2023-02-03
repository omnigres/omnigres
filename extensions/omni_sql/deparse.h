#ifndef OMNI_SQL_DEPARSE_H
#define OMNI_SQL_DEPARSE_H

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

char *omni_sql_deparse_statement(List *stmts);
#endif // OMNI_SQL_DEPARSE_H