/**
 * @file arrays.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <utils/array.h>
#include <utils/lsyscache.h>

#if PG_MAJORVERSION_NUM < 14
// `trim_array` has been extracted from Postgres. It is licensed under the terms of PostgreSQL
// license.
PG_FUNCTION_INFO_V1(trim_array);
/*
 * Trim the last N elements from an array by building an appropriate slice.
 * Only the first dimension is trimmed.
 */
Datum trim_array(PG_FUNCTION_ARGS) {
  ArrayType *v = PG_GETARG_ARRAYTYPE_P(0);
  int n = PG_GETARG_INT32(1);
  int array_length = (ARR_NDIM(v) > 0) ? ARR_DIMS(v)[0] : 0;
  int16 elmlen;
  bool elmbyval;
  char elmalign;
  int lower[MAXDIM];
  int upper[MAXDIM];
  bool lowerProvided[MAXDIM];
  bool upperProvided[MAXDIM];
  Datum result;

  /* Per spec, throw an error if out of bounds */
  if (n < 0 || n > array_length)
    ereport(ERROR, (errcode(ERRCODE_ARRAY_ELEMENT_ERROR),
                    errmsg("number of elements to trim must be between 0 and %d", array_length)));

  /* Set all the bounds as unprovided except the first upper bound */
  memset(lowerProvided, false, sizeof(lowerProvided));
  memset(upperProvided, false, sizeof(upperProvided));
  if (ARR_NDIM(v) > 0) {
    upper[0] = ARR_LBOUND(v)[0] + array_length - n - 1;
    upperProvided[0] = true;
  }

  /* Fetch the needed information about the element type */
  get_typlenbyvalalign(ARR_ELEMTYPE(v), &elmlen, &elmbyval, &elmalign);

  /* Get the slice */
  result = array_get_slice(PointerGetDatum(v), 1, upper, lower, upperProvided, lowerProvided, -1,
                           elmlen, elmbyval, elmalign);

  PG_RETURN_DATUM(result);
}
#endif