// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <omni_sql.h>

PG_FUNCTION_INFO_V1(cascading_query_reduce);

PG_FUNCTION_INFO_V1(cascading_query_final);

#define ARG_STATE 0
#define ARG_NAME 1
#define ARG_QUERY 2

Datum cascading_query_reduce(PG_FUNCTION_ARGS) {
  List *stmts;

  MemoryContext memory_context;
  if (AggCheckCallContext(fcinfo, &memory_context) != 1) {
    ereport(ERROR, errmsg("must be called as a regular aggregate"));
  }

  MemoryContext old_context = MemoryContextSwitchTo(memory_context);

  if PG_ARGISNULL (ARG_NAME) {
    ereport(ERROR, errmsg("name can't be NULL"));
  }

  if PG_ARGISNULL (ARG_QUERY) {
    ereport(ERROR, errmsg("query can't be NULL"));
  }

  // We already have the query
  bool initialized = true;

  if PG_ARGISNULL (ARG_STATE) {
    stmts = omni_sql_parse_statement("SELECT");
    initialized = false;
  } else {
    stmts = (List *)PG_GETARG_POINTER(ARG_STATE);
  }

  text *name = PG_GETARG_TEXT_PP(ARG_NAME);
  char *query = text_to_cstring(PG_GETARG_TEXT_PP(ARG_QUERY));

  RawStmt *original_stmt = castNode(RawStmt, linitial(stmts));
  SelectStmt *original_select_stmt = castNode(SelectStmt, original_stmt->stmt);

  // SELECT * FROM {name}
  SelectStmt *select = makeNode(SelectStmt);
  ResTarget *resTarget = makeNode(ResTarget);
  ColumnRef *star = makeNode(ColumnRef);
  star->fields = list_make1(makeNode(A_Star));
  resTarget->val = (Node *)star;
  select->targetList = list_make1(resTarget);
  RangeVar *from = makeNode(RangeVar);
  from->inh = true;
  from->relname = text_to_cstring(name);
  select->fromClause = list_make1(from);

  if (!initialized) {
    original_stmt->stmt = (Node *)select;
  } else {
    char *previous_name =
        llast_node(CommonTableExpr, original_select_stmt->withClause->ctes)->ctename;

    // SELECT true FROM {previous_name}
    SelectStmt *subQuery = makeNode(SelectStmt);
    ResTarget *subResTarget = makeNode(ResTarget);
    ColumnRef *subRes = makeNode(ColumnRef);
    subResTarget->val = (Node *)star;
    RangeVar *subFrom = makeNode(RangeVar);
    subFrom->inh = true;
    subFrom->relname = previous_name;
    subQuery->fromClause = list_make1(subFrom);

    // NOT EXISTS (SELECT FROM {previous_name})
    BoolExpr *parent = makeNode(BoolExpr);
    parent->boolop = NOT_EXPR;
    SubLink *subLink = makeNode(SubLink);
    subLink->subLinkType = EXISTS_SUBLINK;
    subLink->subselect = (Node *)subQuery;
    parent->args = list_make1(subLink);
    select->whereClause = (Node *)parent;

    // {existing_query} UNION ALL SELECT * FROM {name} WHERE NOT EXISTS (SELECT FROM {previous_name}
    SelectStmt *setop = castNode(SelectStmt, makeNode(SelectStmt));

    setop->op = SETOP_UNION;
    setop->larg = original_select_stmt;
    setop->rarg = select;
    setop->withClause = original_select_stmt->withClause;
    original_select_stmt->withClause = NULL;

    original_stmt->stmt = (Node *)setop;
  }

  omni_sql_add_cte(stmts, name, omni_sql_parse_statement(query), false, false);

  MemoryContextSwitchTo(old_context);

  PG_RETURN_POINTER(stmts);
}

Datum cascading_query_final(PG_FUNCTION_ARGS) {
  List *stmts;

  if PG_ARGISNULL (ARG_STATE) {
    stmts = omni_sql_parse_statement("SELECT");
  } else {
    stmts = (List *)PG_GETARG_POINTER(ARG_STATE);
  }
  PG_RETURN_TEXT_P(cstring_to_text(omni_sql_deparse_statement(stmts)));
}
