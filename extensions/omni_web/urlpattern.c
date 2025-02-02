// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <miscadmin.h>
#include <nodes/execnodes.h>
#include <utils/builtins.h>
#if PG_MAJORVERSION_NUM >= 16
#include <varatt.h>
#endif

#include "urlpattern.h"

PG_FUNCTION_INFO_V1(urlpattern_in);
PG_FUNCTION_INFO_V1(urlpattern_out);
PG_FUNCTION_INFO_V1(urlpattern_matches);
PG_FUNCTION_INFO_V1(urlpattern_match);

Datum urlpattern_in(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  char *input = PG_GETARG_CSTRING(0);
  PG_RETURN_TEXT_P(cstring_to_text(input));
}

Datum urlpattern_out(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  PG_RETURN_CSTRING(text_to_cstring(PG_GETARG_TEXT_PP(0)));
}

Datum urlpattern_matches(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }
  text *pattern = PG_GETARG_TEXT_PP(0);
  text *input = PG_GETARG_TEXT_PP(1);

  text *baseURL = NULL;
  if (!PG_ARGISNULL(2)) {
    baseURL = PG_GETARG_TEXT_PP(2);
  }

  PG_RETURN_BOOL(matches(VARDATA_ANY(pattern), VARSIZE_ANY_EXHDR(pattern), VARDATA_ANY(input),
                         VARSIZE_ANY_EXHDR(input), baseURL ? VARDATA_ANY(baseURL) : NULL,
                         baseURL ? VARSIZE_ANY_EXHDR(baseURL) : 0));
}

Datum urlpattern_match(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }
  text *pattern = PG_GETARG_TEXT_PP(0);
  text *input = PG_GETARG_TEXT_PP(1);

  text *baseURL = NULL;
  if (!PG_ARGISNULL(2)) {
    baseURL = PG_GETARG_TEXT_PP(2);
  }

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  bool matches =
      match_resultset(rsinfo, VARDATA_ANY(pattern), VARSIZE_ANY_EXHDR(pattern), VARDATA_ANY(input),
                      VARSIZE_ANY_EXHDR(input), baseURL ? VARDATA_ANY(baseURL) : NULL,
                      baseURL ? VARSIZE_ANY_EXHDR(baseURL) : 0);

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);

  PG_RETURN_NULL();
}