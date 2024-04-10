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
