#include <cppgres.hpp>

extern "C" {
PG_MODULE_MAGIC;

#include <omni/omni_v0.h>
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_differential", .version = EXT_VERSION,
                 .identity = "d917692d-24dc-4629-8c14-73e649f629e1");

#include "postgres.h"
#include "nodes/pg_list.h"
#include "parser/parser.h"     // pg_parse_query
#include "tcop/tcopprot.h"     // pg_analyze_and_rewrite
#include "utils/memutils.h"    // MemoryContext
#include "utils/snapmgr.h"     // PushActiveSnapshot/GetTransactionSnapshot
#include "nodes/nodeFuncs.h"   // castNode / lfirst_node
#include "utils/ruleutils.h"
#include "nodes/makefuncs.h"
#include "deparse.h"
#include "common/cryptohash.h"
#include "common/sha2.h"
#include "parser/parsetree.h"

#include "catalog/pg_description.h"
#include "catalog/pg_class_d.h"
#include "utils/builtins.h"

#include "utils/fmgroids.h"
}

#include <map>

int64_t current_cid_impl() {
  return GetCurrentCommandId(false);
}
postgres_function(current_cid, current_cid_impl);

static void compute_hash(const char *input, uint8 (*hash)[PG_SHA256_DIGEST_LENGTH]) {
  pg_cryptohash_ctx *ctx = pg_cryptohash_create(PG_SHA256);
  if (ctx == nullptr) { elog(ERROR, "Could not create cryptohash context"); }
  if (pg_cryptohash_init(ctx) != 0) {
    elog(ERROR, "Could not initialize cryptohash context: %s", pg_cryptohash_error(ctx));
  }
  bool is_nonnull = input != nullptr;
  if (pg_cryptohash_update(ctx, (const uint8 *) &(is_nonnull), sizeof(is_nonnull)) != 0) {
    elog(ERROR, "Could not update cryptohash context: %s", pg_cryptohash_error(ctx));
  }
  if (is_nonnull) {
    auto input_length = strlen(input);
    if (pg_cryptohash_update(ctx, (const uint8 *) &input_length, sizeof(input_length)) != 0) {
      elog(ERROR, "Could not update cryptohash context: %s", pg_cryptohash_error(ctx));
    }
    if (pg_cryptohash_update(ctx, (const uint8 *) input, input_length) != 0) {
      elog(ERROR, "Could not update cryptohash context: %s", pg_cryptohash_error(ctx));
    }
  }
  if (pg_cryptohash_final(ctx, (uint8 *) hash, sizeof(*hash)) != 0) {
    elog(ERROR, "Could not finalize cryptohash context: %s", pg_cryptohash_error(ctx));
  }
  pg_cryptohash_free(ctx);
}

static const char *gensym(char * sql) {
  static const char *chars = "abcdefghijklmnopqrstuvwxyz234567";
  uint8 hash[PG_SHA256_DIGEST_LENGTH];
  compute_hash(sql, &hash);
  hash[0] = chars[hash[0] & 0b00001111];
  for (size_t i = 1; i < sizeof(hash); i++) {
    hash[i] = chars[hash[i] & 0b00011111];
  }
  hash[16] = '\0'; // truncate to 16 chars
  return pstrdup((const char *)hash);
}

// This does not properly quote identifiers with special characters.
static char *result_type_str(Query *query) {
  // get the clean TupleDesc for the query
  TupleDesc tupdesc = ExecCleanTypeFromTL(query->targetList);
  StringInfoData buf;
  initStringInfo(&buf);
  appendStringInfoChar(&buf, '(');
  for (int i = 0; i < tupdesc->natts; i++) {
      Form_pg_attribute a = TupleDescAttr(tupdesc, i);
      if (a->attisdropped) continue;
      if (buf.len > 1) appendStringInfoString(&buf, ", ");
      appendStringInfo(&buf, "%s %s",
          quote_identifier(NameStr(a->attname)),
          format_type_be(a->atttypid));
  }
  appendStringInfoChar(&buf, ')');
  return buf.data;
}

static char *get_table_comment(Oid relid)
{
    NameData cat;                      /* type 'name' */
    namestrcpy(&cat, "pg_class");      /* catalog name in pg_catalog */
    FmgrInfo    flinfo;
    fmgr_info(F_OBJ_DESCRIPTION_OID_NAME, &flinfo);
    LOCAL_FCINFO(fcinfo, 2);
    InitFunctionCallInfoData(*fcinfo, &flinfo, 2, InvalidOid, NULL, NULL);

    fcinfo->args[0].value = ObjectIdGetDatum(relid);
    fcinfo->args[0].isnull = false;
    fcinfo->args[1].value = NameGetDatum(&cat);
    fcinfo->args[1].isnull = false;

    Datum result = FunctionCallInvoke(fcinfo);
    if (fcinfo->isnull) { return nullptr; }
    return text_to_cstring(DatumGetTextPP(result));  /* palloc’d in current context */
}

static List *parse_and_rewrite(const char *sql_c_str) {
  List *queries_rewritten = NIL;
  ListCell *lc;
  foreach (lc, pg_parse_query(sql_c_str)) {
    List *queries = ::pg_analyze_and_rewrite_fixedparams(
        lfirst_node(RawStmt, lc), sql_c_str,
        nullptr, 0, nullptr
    );
    if (list_length(queries) != 1) {
      elog(ERROR, "Rewriting did not return exactly one query");
    }
    queries_rewritten = lappend(queries_rewritten, linitial(queries));
  }
  return queries_rewritten;
}

static void deparse_query(Query *query, char **out_sql, RawStmt **out_raw_stmt) {
//      List *spath = fetch_search_path(false);
//      if (spath != NIL) { elog(ERROR, "search_path must be empty"); }
//      list_free(spath);
      *out_sql = pg_get_querydef(query, false);
      List *raw_stmts = pg_parse_query(*out_sql);
      if (list_length(raw_stmts) != 1) {
        elog(ERROR, "Omni_diff internal error: unexpected number of statements after re-parsing");
      }
      *out_raw_stmt = linitial_node(RawStmt, raw_stmts);
}

static void get_table_oids_from_query(Query *query, Oid *out_left, Oid *out_right)
{
    FromExpr   *from;
    Node       *fromnode;
    JoinExpr   *join;
    RangeTblRef *lref;
    RangeTblRef *rref;
    RangeTblEntry *lrte;
    RangeTblEntry *rrte;

    if (query == nullptr) { elog(ERROR, "NULL Query pointer"); }
    if (out_left == nullptr || out_right == nullptr) { elog(ERROR, "Output Oid pointers must not be NULL"); }
    if (query->commandType != CMD_SELECT) { elog(ERROR, "Only SELECT queries are supported"); }
    if (query->jointree == nullptr || !IsA(query->jointree, FromExpr)) { elog(ERROR, "Query has no valid jointree/FromExpr"); }
    from = (FromExpr *) query->jointree;
    if (list_length(from->fromlist) != 1) { elog(ERROR, "FROM clause must have exactly one join"); }
    fromnode = (Node *) linitial(from->fromlist);
    if (!IsA(fromnode, JoinExpr)) { elog(ERROR, "The single FROM item must be an explicit JOIN"); }
    join = (JoinExpr *) fromnode;
    if (join->jointype != JOIN_INNER) {elog(ERROR, "The join must be an INNER JOIN");}
    /* Join must be between two base tables (no subqueries, no nested joins) */
    if (!IsA(join->larg, RangeTblRef) || !IsA(join->rarg, RangeTblRef))
      { elog(ERROR, "INNER JOIN must be between two base tables"); }
    lref = (RangeTblRef *) join->larg;
    rref = (RangeTblRef *) join->rarg;

    /* Fetch RTEs and confirm they are base relations */
    lrte = rt_fetch(lref->rtindex, query->rtable);
    rrte = rt_fetch(rref->rtindex, query->rtable);

    if (lrte == nullptr || rrte == nullptr) { elog(ERROR, "Failed to fetch range table entries"); }

    if (lrte->rtekind != RTE_RELATION || rrte->rtekind != RTE_RELATION) {
      elog(ERROR, "JOIN operands must be base tables, not subqueries or functions");
    }
    if (lrte->relkind != RELKIND_RELATION) { elog(ERROR, "Not a regular relation"); }
    if (rrte->relkind != RELKIND_RELATION) { elog(ERROR, "Not a regular relation"); }
    *out_left = lrte->relid;
    *out_right = rrte->relid;
}

static void get_range_var_nodes(SelectStmt *selectStmt, RangeVar **out_left, RangeVar **out_right) {
  if (list_length(selectStmt->fromClause) != 1) {
    elog(ERROR, "The FROM clause must have exactly one item");
  }
  auto fromItem = linitial( selectStmt->fromClause);
  if (!IsA(fromItem, JoinExpr)) {
    elog(ERROR, "The FROM item must be a JOIN");
  }
  auto joinExpr = (JoinExpr *) fromItem;
  if (joinExpr->jointype != JOIN_INNER) {
    elog(ERROR, "JOIN type must be INNER JOIN");
  }
  if (!IsA(joinExpr->larg, RangeVar)) {
    elog(ERROR, "Left side of the JOIN must be a range variable");
  }
  if (!IsA(joinExpr->rarg, RangeVar)) {
    elog(ERROR, "Right side of the JOIN must be a range variable");
  }
  *out_left = castNode(RangeVar, joinExpr->larg);
  *out_right = castNode(RangeVar, joinExpr->rarg);
}

static void redirect_range_var_in_place(RangeVar *rangeVar, const char * name) {
  if (rangeVar->relname == nullptr) {
    elog(ERROR, "relname must be non-null");
  }
  rangeVar->schemaname = nullptr;
  if (rangeVar->alias == nullptr) {
    rangeVar->alias = makeNode(Alias);
    rangeVar->alias->aliasname = pstrdup(rangeVar->relname);
  }
  rangeVar->relname = pstrdup(name); // is dup needed?
}

static void redirect_join_range_vars_in_place(SelectStmt *selectStmt, const char * left_name, const char * right_name) {
  RangeVar *rv_left = nullptr; RangeVar *rv_right = nullptr;
  get_range_var_nodes(selectStmt, &rv_left, &rv_right);
  if (left_name) { redirect_range_var_in_place(rv_left, left_name); }
  if (right_name) { redirect_range_var_in_place(rv_right, right_name); }
}

static SelectStmt *redirect_join_range_vars(SelectStmt const *selectStmt, const char * left_name, const char * right_name) {
  auto selectStmtCopy = (SelectStmt *) copyObject(selectStmt);
  redirect_join_range_vars_in_place(selectStmtCopy, left_name, right_name);
  return selectStmtCopy;
}

static SelectStmt *selectstmt_setop(SelectStmt *left, SelectStmt *right, SetOperation op, bool all) {
    auto u = makeNode(SelectStmt);
    u->larg = left;
    u->rarg = right;
    u->op   = op;
    u->all  = all;
    return u;
}

static SelectStmt *union_all(SelectStmt *l, SelectStmt *r) {
  return selectstmt_setop(l, r, SETOP_UNION, true);
}
static SelectStmt *union_all(SelectStmt *l1, SelectStmt *l2, SelectStmt *r) {
  return union_all(union_all(l1, l2), r);
}
static SelectStmt *union_all(SelectStmt *l1, SelectStmt *l2, SelectStmt *l3, SelectStmt *r) {
  return union_all(union_all(l1, l2, l3), r);
}

static SelectStmt *except_all(SelectStmt *l, SelectStmt *r) {
  return selectstmt_setop(l, r, SETOP_EXCEPT, true);
}

static char *diff_def_body_str(Query *query) {
  Oid left_table_oid = InvalidOid;
  Oid right_table_oid = InvalidOid;
  get_table_oids_from_query(query, &left_table_oid, &right_table_oid);
  if (!OidIsValid(left_table_oid) || !OidIsValid(right_table_oid)) {
    elog(ERROR, "Could not determine table OIDs");
  }
  // we now retrieve the comments for these tables
  char * left_table_uuid = get_table_comment(left_table_oid);
  char * right_table_uuid = get_table_comment(right_table_oid);
  if (left_table_uuid == nullptr || right_table_uuid == nullptr) {
    elog(ERROR, "Both tables must have comments");
  }

  char * rewritten_sql = nullptr;
  RawStmt *rewritten_raw_stmt = nullptr;
  deparse_query(query, &rewritten_sql, &rewritten_raw_stmt);
  if (rewritten_raw_stmt->stmt->type != T_SelectStmt) {
    elog(ERROR, "Only SELECT statements are supported");
  }
  auto selectStmt = (SelectStmt *) rewritten_raw_stmt->stmt;

  const char * sym_base = gensym(rewritten_sql);
  const char * gain_l = psprintf("%s_%s", sym_base, "gain_left");
  const char * loss_l = psprintf("%s_%s", sym_base, "loss_left");
  const char * gain_r = psprintf("%s_%s", sym_base, "gain_right");
  const char * loss_r = psprintf("%s_%s", sym_base, "loss_right");

  auto new_stmt = except_all(
      union_all(
          redirect_join_range_vars(selectStmt, nullptr, gain_r),
          redirect_join_range_vars(selectStmt, gain_l, nullptr),
          redirect_join_range_vars(selectStmt, loss_l, gain_r),
          redirect_join_range_vars(selectStmt, gain_l, loss_r)
      ),
      union_all(
          redirect_join_range_vars(selectStmt, nullptr, loss_r),
          redirect_join_range_vars(selectStmt, loss_l, nullptr),
          redirect_join_range_vars(selectStmt, gain_l, gain_r)
      )
  );
  // now we add a WITH clause: it will map gain_l to 'select * from omni_history.gain_since($1)'
  // and loss_l to 'select * from omni_history.loss_since($1)'
  // and similarly for gain_r and loss_r
  WithClause *withClause = makeNode(WithClause);
  withClause->ctes = list_make4(
    makeNode(CommonTableExpr),
    makeNode(CommonTableExpr),
    makeNode(CommonTableExpr),
    makeNode(CommonTableExpr)
  );
  withClause->recursive = false;
  withClause->location = -1;
  {
    auto cte = linitial_node(CommonTableExpr, withClause->ctes);
    cte->ctename = pstrdup(gain_l);
    cte->ctequery = linitial_node(RawStmt, pg_parse_query(psprintf(
      "SELECT * FROM omni_history.%s($1)",
        quote_identifier(psprintf("%s_gain_since", left_table_uuid)))))->stmt;
    cte->location = -1;
  }
  {
    auto cte = (CommonTableExpr *) lsecond(withClause->ctes);
    cte->ctename = pstrdup(loss_l);
    cte->ctequery = linitial_node(RawStmt, pg_parse_query(psprintf(
      "SELECT * FROM omni_history.%s($1)",
      quote_identifier(psprintf("%s_loss_since", left_table_uuid)))))->stmt;
    cte->location = -1;
  }
  {
    auto cte = (CommonTableExpr *) lthird(withClause->ctes);
    cte->ctename = pstrdup(gain_r);
    cte->ctequery = linitial_node(RawStmt, pg_parse_query(psprintf(
      "SELECT * FROM omni_history.%s($1)",
      quote_identifier(psprintf("%s_gain_since", right_table_uuid)))))->stmt;
    cte->location = -1;
  }
  {
    auto cte = (CommonTableExpr *) lfourth(withClause->ctes);
    cte->ctename = pstrdup(loss_r);
    cte->ctequery = linitial_node(RawStmt, pg_parse_query(psprintf(
      "SELECT * FROM omni_history.%s($1)",
      quote_identifier(psprintf("%s_loss_since", right_table_uuid)))))->stmt;
    cte->location = -1;
  }
  new_stmt->withClause = withClause;

  auto new_raw_stmt = makeNode(RawStmt);
  new_raw_stmt->stmt = (Node *) new_stmt;
  new_raw_stmt->stmt_location = -1;
  new_raw_stmt->stmt_len = 0;

  List *stmts = list_make1(new_raw_stmt);

  return omni_sql_deparse_statement(stmts);
}

static Query *get_single_rewritten_query(std::string_view sql) {
  List *rewritten_queries = nullptr;
  {
    char * sql_c_str = pstrdup(std::string(sql).c_str()); // std::string d-tor before potential longjmp
    rewritten_queries = parse_and_rewrite(sql_c_str);
  }
  if (list_length(rewritten_queries) != 1) {
    elog(ERROR, "Expected exactly one query after rewriting");
  }
  return linitial_node(Query, rewritten_queries);
}

std::string diff_def_body_impl(std::string_view sql) {
  auto query = get_single_rewritten_query(sql);
  return diff_def_body_str(query);
}
postgres_function(diff_def_body, diff_def_body_impl);

std::string diff_def_impl(std::string_view sql, std::string_view fun_name) {
  auto query = get_single_rewritten_query(sql);
  auto fun_name_c_str = pstrdup(std::string(fun_name).c_str());
  return psprintf(
      "create function %s(snapshot omni_differential.omni_snapshot)\n"
      "returns table %s\n"
      "begin atomic\n"
      "%s;\n"
      "end",
      quote_identifier(fun_name_c_str),
      result_type_str(query),
      diff_def_body_str(query)
  );
}
postgres_function(diff_def, diff_def_impl);
