/**
 * @file omni_web.c
 *
 */
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <catalog/pg_type.h>
#include <utils/array.h>
#include <utils/builtins.h>

#include <uriparser/Uri.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(parse_query_string);

#define ARG_QUERY_STRING 0

Datum parse_query_string(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(ARG_QUERY_STRING)) {
    PG_RETURN_NULL();
  }
  char *query = text_to_cstring(PG_GETARG_TEXT_PP(ARG_QUERY_STRING));

  UriQueryListA *queryList;
  int itemCount;
  int rc = uriDissectQueryMallocA(&queryList, &itemCount, query, query + strlen(query));
  if (rc != URI_SUCCESS) {
    ereport(ERROR, errmsg("failed parsing"));
  }

  Datum *elements = palloc_array(Datum, itemCount * 2);
  bool *nulls = palloc_array(bool, itemCount * 2);

  UriQueryListA *currentQueryParam = queryList;
  int i = 0;
  while (currentQueryParam != NULL) {
    elements[i] =
        PointerGetDatum(currentQueryParam->key ? cstring_to_text(currentQueryParam->key) : NULL);
    nulls[i] = currentQueryParam->key == NULL;
    elements[i + 1] = PointerGetDatum(
        currentQueryParam->value ? cstring_to_text(currentQueryParam->value) : NULL);
    nulls[i + 1] = currentQueryParam->value == NULL;
    currentQueryParam = currentQueryParam->next;
    i += 2;
  }
  uriFreeQueryListA(queryList);

  ArrayType *array = construct_md_array(elements, nulls, 1, (int[1]){itemCount * 2}, (int[1]){1},
                                        TEXTOID, -1, false, TYPALIGN_INT);
  PG_RETURN_ARRAYTYPE_P(array);
}