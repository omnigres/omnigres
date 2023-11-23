/**
 * @file omni_web.c
 *
 */
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <catalog/pg_type.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <utils/array.h>
#include <utils/builtins.h>

#include <uriparser/Uri.h>

#include <libpgaug.h>

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

CACHED_OID(uri);

#define URI_TUPLE_SCHEME 0
#define URI_TUPLE_USER_INFO 1
#define URI_TUPLE_HOST 2
#define URI_TUPLE_PATH 3
#define URI_TUPLE_PORT 4
#define URI_TUPLE_QUERY 5
#define URI_TUPLE_FRAGMENT 6

PG_FUNCTION_INFO_V1(text_to_uri);

Datum text_to_uri(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  UriUriA uri;
  const char *error_pos;
  char *string = text_to_cstring(PG_GETARG_TEXT_PP(0));
  if (uriParseSingleUriA(&uri, string, &error_pos) != URI_SUCCESS) {
    ereport(ERROR, errmsg("error parsing URI"),
            errdetail("error starting at position %ld: %s", error_pos - string, error_pos));
  }
  TupleDesc tuplesdesc = TypeGetTupleDesc(uri_oid(), NULL);
  BlessTupleDesc(tuplesdesc);

  bool nulls[7] = {
      [URI_TUPLE_SCHEME] = uri.scheme.first == NULL,
      [URI_TUPLE_USER_INFO] = uri.userInfo.first == NULL,
      [URI_TUPLE_HOST] = uri.hostText.first == NULL && uri.hostData.ipFuture.first == NULL,
      [URI_TUPLE_PATH] = uri.pathHead == NULL,
      [URI_TUPLE_PORT] = uri.portText.first == NULL,
      [URI_TUPLE_QUERY] = uri.query.first == NULL,
      [URI_TUPLE_FRAGMENT] = uri.fragment.first == NULL,
  };

  Datum values[7] = {
      [URI_TUPLE_SCHEME] = nulls[URI_TUPLE_SCHEME]
                               ? 0
                               : PointerGetDatum(cstring_to_text_with_len(
                                     uri.scheme.first, uri.scheme.afterLast - uri.scheme.first)),
      [URI_TUPLE_USER_INFO] =
          nulls[URI_TUPLE_USER_INFO]
              ? 0
              : PointerGetDatum(cstring_to_text_with_len(
                    uri.userInfo.first, uri.userInfo.afterLast - uri.userInfo.first)),
      [URI_TUPLE_HOST] = nulls[URI_TUPLE_HOST] ? 0
                                               : PointerGetDatum(cstring_to_text_with_len(
                                                     uri.hostText.first,
                                                     uri.hostText.afterLast - uri.hostText.first)),
      [URI_TUPLE_PORT] =
          nulls[URI_TUPLE_PORT] ? 0 : Int32GetDatum(strtol(uri.portText.first, NULL, 10)),
      [URI_TUPLE_QUERY] = nulls[URI_TUPLE_QUERY]
                              ? 0
                              : PointerGetDatum(cstring_to_text_with_len(
                                    uri.query.first, uri.query.afterLast - uri.query.first)),
      [URI_TUPLE_FRAGMENT] =
          nulls[URI_TUPLE_FRAGMENT]
              ? 0
              : PointerGetDatum(cstring_to_text_with_len(
                    uri.fragment.first, uri.fragment.afterLast - uri.fragment.first))};

  if (!nulls[URI_TUPLE_PATH]) {
    int len = 0;
    UriPathSegmentA *segment = uri.pathHead;
    while (segment != NULL) {
      if (segment->next == NULL) {
        len = segment->text.afterLast - uri.pathHead->text.first;
        break;
      }
      if (segment->next->text.first == segment->next->text.afterLast) {
        len = segment->text.afterLast - uri.pathHead->text.first + 1;
        break;
      }
      segment = segment->next;
    }
    values[URI_TUPLE_PATH] =
        PointerGetDatum(cstring_to_text_with_len(uri.pathHead->text.first, len));
  }

  HeapTuple uri_tuple = heap_form_tuple(tuplesdesc, values, nulls);

  uriFreeUriMembersA(&uri);

  PG_RETURN_DATUM(HeapTupleGetDatum(uri_tuple));
}