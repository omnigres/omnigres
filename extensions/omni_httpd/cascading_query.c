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

struct rename {
  char *from;
  size_t len;
  char *to;
};

static bool renaming_walker(Node *node, struct rename *rename) {
  if (node != NULL) {
    if (IsA(node, RangeVar)) {
      RangeVar *rangeVar = castNode(RangeVar, node);
      char *relname = rangeVar->relname;
      if (strlen(relname) == rename->len && strncasecmp(relname, rename->from, rename->len) == 0) {
        rangeVar->alias = makeNode(Alias);
        rangeVar->alias->aliasname = rename->from;
        rangeVar->relname = rename->to;
      }
    } else {
      return raw_expression_tree_walker(node, renaming_walker, rename);
    }
  }
  return false;
}

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
    setop->all = true;
    original_select_stmt->withClause = NULL;

    original_stmt->stmt = (Node *)setop;
  }

  // Get given query's CTEs
  List *parsed_query = omni_sql_parse_statement(query);
  WithClause **withClause;
  if (omni_sql_get_with_clause(linitial(parsed_query), &withClause) && (*withClause != NULL) &&
      (*withClause)->ctes != NULL) {

    // Get accumulated statement's with clause
    WithClause **stmtWithClause;
    if (!omni_sql_get_with_clause(linitial(stmts), &stmtWithClause) || *stmtWithClause == NULL) {
      WithClause *new_with = makeNode(WithClause);
      new_with->recursive = false;
      *stmtWithClause = new_with;
    }

    const ListCell *lc = NULL;
    // Rename every sub-CTE to include the CTE name
    foreach (lc, (*withClause)->ctes) {
      CommonTableExpr *cte = castNode(CommonTableExpr, lfirst(lc));
      char *from = cte->ctename;
      // Rename references by aliasing CTE references. To mitigate the risk of the name
      // collision with a relation defined outside of the query, we'll prefix them.
      // But a more sophisticated approach can be used to detect collisions (TODO)
      cte->ctename = psprintf("__omni_httpd_%s_%s", text_to_cstring(name), cte->ctename);
      struct rename rename = {.from = from, .len = strlen(from), .to = cte->ctename};
      raw_expression_tree_walker(castNode(RawStmt, linitial(parsed_query))->stmt, renaming_walker,
                                 &rename);
    }
    // Move the CTEs to the top level
    if ((*stmtWithClause)->ctes == NULL) {
      (*stmtWithClause)->ctes = (*withClause)->ctes;
    } else {
      (*stmtWithClause)->ctes = list_concat((*stmtWithClause)->ctes, (*withClause)->ctes);
    }
    (*withClause)->ctes = NULL;
  }

  omni_sql_add_cte(stmts, name, parsed_query, false, false);

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
