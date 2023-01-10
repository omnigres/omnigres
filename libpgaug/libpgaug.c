#include "libpgaug.h"

void *pgaug_alloc(size_t size) { return palloc(size); }

void *pgaug_calloc(uintptr_t num, uintptr_t count) {
  return palloc0(num * count);
}

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