#include <postgres.h>
#include <utils/memutils.h>

#include "libgluepg_yyjson.h"

static void *yyjson_malloc(__attribute__((unused)) void *ctx, size_t size) { return palloc(size); }

static void pgfree(void *ptr) {
  if (ptr != NULL && GetMemoryChunkContext(ptr) != NULL) {
    pfree(ptr);
  }
}

static void *pgrealloc(void *ptr, unsigned long size) {
  if (ptr != NULL) {
    return repalloc(ptr, size);
  } else {
    return palloc(size);
  }
}

static void *yyjson_realloc(__attribute__((unused)) void *ctx, void *ptr, size_t size) {
  return pgrealloc(ptr, size);
}

static void yyjson_free(void *ctx, void *ptr) { pgfree(ptr); }

yyjson_alc gluepg_yyjson_allocator = {
    .ctx = NULL,
    .malloc = yyjson_malloc,
    .free = yyjson_free,
    .realloc = yyjson_realloc,
};