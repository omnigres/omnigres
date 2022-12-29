// clang-format off
#include <postgres.h>
#include <miscadmin.h>
// clang-format on

#include <stddef.h>
#include <utils/memutils.h>

#include <libpgaug/framac.h>

#include "libgluepg_curl.h"

void *gluepg_curl_pcalloc(uintptr_t num, uintptr_t count) {
  return palloc0(num * count);
}

void gluepg_curl_pgfree(void *ptr) {
  if (ptr != NULL && GetMemoryChunkContext(ptr) != NULL) {
    pfree(ptr);
  }
}

void *gluepg_curl_pgrealloc(void *ptr, uintptr_t size) {
  if (ptr != NULL) {
    return repalloc(ptr, size);
  } else {
    return palloc(size);
  }
}

static bool curl_initialized = false;

void gluepg_curl_init() {
  if (!curl_initialized) {
    curl_global_init_mem(CURL_GLOBAL_DEFAULT, palloc, gluepg_curl_pgfree,
                         gluepg_curl_pgrealloc, pstrdup, gluepg_curl_pcalloc);
    curl_initialized = true;
  }
}

void gluepg_curl_buffer_init(gluepg_curl_buffer *buf) {
  buf->size = 0;
  buf->allocated = 0;
  buf->data = palloc(0);
}
void gluepg_curl_buffer_reset(gluepg_curl_buffer *buf) { buf->size = 0; }

/*@
   requires \valid_read(data+(0..size * nmemb));
   requires \valid(mem);
   requires \valid(mem->data+(0..mem->allocated));
   requires alloc_size_is_valid(mem->size + size * nmemb + 1);
   requires mem->size <= mem->allocated;
   ensures \result == size * nmemb;
   ensures mem->size == \old(mem->size) + \result;
   behavior underallocated:
     assumes mem->allocated < mem->size + size*nmemb + 1;
     ensures mem->allocated == \old(mem->size) + size*nmemb + 1;
   behavior properly_or_over_allocated:
     assumes mem->allocated >= mem->size + size*nmemb + 1;
     requires \separated(&(mem->data[mem->size])+(0..size * nmemb),
               data+(0..size * nmemb));
     ensures mem->allocated == \old(mem->allocated);
   complete behaviors;
   disjoint behaviors;
  */
static inline size_t
gluepg_curl_buffer_write_impl(char *restrict data, size_t size, size_t nmemb,
                              gluepg_curl_buffer *restrict mem) {
#ifndef __FRAMAC__
  CHECK_FOR_INTERRUPTS();
#endif
  size_t realsize = size * nmemb;

  uintptr_t new_size = mem->size + realsize + 1;
  if (mem->allocated < new_size) {
    mem->data = (char *)repalloc((void *)mem->data, new_size);
    mem->allocated = new_size;
  }

  memcpy(&mem->data[mem->size], data, realsize);
  mem->size += realsize;
  mem->data[mem->size] = 0;

  return realsize;
}

size_t gluepg_curl_buffer_write(void *data, size_t size, size_t nmemb,
                                void *buffer) {
  return gluepg_curl_buffer_write_impl((char *)data, size, nmemb,
                                       (gluepg_curl_buffer *)buffer);
}
