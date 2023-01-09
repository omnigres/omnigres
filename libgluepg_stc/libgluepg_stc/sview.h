#ifndef LIBGLUEPG_STC_SVIEW_H
#define LIBGLUEPG_STC_SVIEW_H

#include <postgres.h>
#include <utils/memutils.h>

#include <stc/cstr.h>
#include <stc/csview.h>

STC_API char *pstring_from_csview(csview v) {
  size_t sz = csview_size(v);
  char *result = (char *)MemoryContextAlloc(CurrentMemoryContext, sz + 1);
  memcpy(result, v.str, sz);
  result[v.size] = 0;
  return result;
}

#endif //  LIBGLUEPG_STC_SVIEW_H