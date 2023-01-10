#ifndef LIBGLUEPG_STC_STR_H
#define LIBGLUEPG_STC_STR_H

#include <postgres.h>
#include <utils/memutils.h>

#include <stc/cstr.h>

STC_API char *pstring_from_cstr(cstr *str) {
  char *result = pstrdup(cstr_data(str));
  cstr_drop(str);
  return result;
}

#endif //  LIBGLUEPG_STC_STR_H