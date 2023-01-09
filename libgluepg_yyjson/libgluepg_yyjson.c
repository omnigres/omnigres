#include <postgres.h>
#include <utils/memutils.h>

#include <libpgaug.h>

#include "libgluepg_yyjson.h"

static void *yyjson_alloc(__attribute__((unused)) void *ctx, size_t size) {
  return pgaug_alloc(size);
}

static void *yyjson_realloc(__attribute__((unused)) void *ctx, void *ptr,
                            size_t size) {
  return pgaug_realloc(ptr, size);
}

static void yyjson_free(__attribute__((unused)) void *ctx, void *ptr) {
  pgaug_free(ptr);
}

yyjson_alc gluepg_yyjson_allocator = {
    .ctx = NULL,
    .malloc = yyjson_alloc,
    .free = yyjson_free,
    .realloc = yyjson_realloc,
};