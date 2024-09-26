#include <postgres.h>

#include <common/hashfn.h>
#include <lib/dshash.h>

static dshash_hash dshash_string_hash(const void *v, size_t size, void *arg) {
  return string_hash(v, size);
}

#if PG_MAJORVERSION_NUM >= 17
static void dshash_string_copy(void *dest, const void *src, size_t size, void *arg) {
  memcpy(dest, src, size);
}
#endif

#if PG_MAJORVERSION_NUM < 17
static int dshash_strcmp(const void *a, const void *b, size_t size, void *arg) {
  return strcmp(a, b);
}
#endif