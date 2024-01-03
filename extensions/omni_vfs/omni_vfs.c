/**
 * @file omni_vfs.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <catalog/pg_enum.h>
#include <executor/spi.h>
#include <utils/builtins.h>
#include <utils/syscache.h>

PG_MODULE_MAGIC;

#include "libpgaug.h"

#include "pg_path.h"

CACHED_OID(omni_vfs_types_v1, file_kind);
CACHED_OID(omni_vfs_types_v1, file_info);
CACHED_ENUM_OID(file_kind, file)
CACHED_ENUM_OID(file_kind, dir)

PG_FUNCTION_INFO_V1(canonicalize_path_pg);

Datum canonicalize_path_pg(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }

  text *path = PG_GETARG_TEXT_PP(0);

  bool absolute = false;
  if (!PG_ARGISNULL(1)) {
    absolute = PG_GETARG_BOOL(1);
  }

  char *cpath;
  // Empty path becomes '/'
  if (VARSIZE_ANY_EXHDR(path) > 0) {
    cpath = text_to_cstring(path);
    canonicalize_path(cpath);
    // Make it absolute if requested
    if (absolute) {
      if (cpath[0] != '/') {
        char *cpath1 = palloc(strlen(cpath) + 2);
        cpath1[0] = '/';
        strncpy(cpath1 + 1, cpath, strlen(cpath) + 1);
        cpath = cpath1;
      }
    }
  } else {
    cpath = "/";
  }

  PG_RETURN_TEXT_P(cstring_to_text(cpath));
}

PG_FUNCTION_INFO_V1(file_basename);

Datum file_basename(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }

  text *path = PG_GETARG_TEXT_PP(0);
  char *cpath = text_to_cstring(path);

  char *last_dir_sep = last_dir_separator(cpath);

  if (last_dir_sep == NULL) {
    PG_RETURN_TEXT_P(path);
  }

  PG_RETURN_TEXT_P(cstring_to_text(last_dir_sep + 1));
}

PG_FUNCTION_INFO_V1(file_dirname);

Datum file_dirname(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }

  text *path = PG_GETARG_TEXT_PP(0);
  char *cpath = text_to_cstring(path);

  char *last_dir_sep = last_dir_separator(cpath);

  if (last_dir_sep == NULL) {
    PG_RETURN_TEXT_P(path);
  }
  int len = last_dir_sep - cpath;
  if (len > 0) {
    PG_RETURN_TEXT_P(cstring_to_text_with_len(cpath, last_dir_sep - cpath));
  } else {
    PG_RETURN_TEXT_P(cstring_to_text("/"));
  }
}