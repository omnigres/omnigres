#include "libpgaug.h"

void *pgaug_alloc(size_t size) { return palloc(size); }

void *pgaug_calloc(uintptr_t num, uintptr_t count) { return palloc0(num * count); }

void pgaug_free(void *ptr) {
  if (ptr != NULL && GetMemoryChunkContext(ptr) != NULL) {
    pfree(ptr);
  }
}

void *pgaug_realloc(void *ptr, uintptr_t size) {
  if (ptr != NULL) {
    return repalloc(ptr, size);
  } else {
    return palloc(size);
  }
}

void __with_temp_memcxt_cleanup(struct __with_temp_memcxt *s) {
  if (CurrentMemoryContext == s->new) {
    CurrentMemoryContext = s->old;
  }
  MemoryContextDelete(s->new);
}

#include <catalog/pg_collation_d.h>
#include <utils/varlena.h>

int namecmp(Name arg1, Name arg2, Oid collid) {
  /*
   * This code is licensed under the terms of PostgreSQL license
   *
   * Portions Copyright (c) 1996-2023, PostgreSQL Global Development Group
   * Portions Copyright (c) 1994, Regents of the University of California
   */
  /* Fast path for common case used in system catalogs */
  if (collid == C_COLLATION_OID)
    return strncmp(NameStr(*arg1), NameStr(*arg2), NAMEDATALEN);

  /* Else rely on the varstr infrastructure */
  return varstr_cmp(NameStr(*arg1), strlen(NameStr(*arg1)), NameStr(*arg2), strlen(NameStr(*arg2)),
                    collid);
}