#include <postgres.h>

#include <common/hashfn.h>
#include <lib/dshash.h>

static dshash_hash dshash_string_hash(const void *v, size_t size, void *arg) {
  return string_hash(v, size);
}

static int dshash_strcmp(const void *a, const void *b, size_t size, void *arg) {
  return strcmp(a, b);
}
