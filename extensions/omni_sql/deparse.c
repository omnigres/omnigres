// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include "lib/stringinfo.h"
#include "nodes/nodes.h"
#include "nodes/parsenodes.h"

void omni_sql_deparseRawStmt(StringInfo str, RawStmt *raw_stmt);

char *omni_sql_deparse_statement(List *stmts) {
  char *result;
  StringInfoData str;
  ListCell *lc;

  initStringInfo(&str);

  foreach (lc, stmts) {
    omni_sql_deparseRawStmt(&str, castNode(RawStmt, lfirst(lc)));
    if (lnext(stmts, lc))
      appendStringInfoString(&str, "; ");
  }
  result = strdup(str.data);

  return result;
}
