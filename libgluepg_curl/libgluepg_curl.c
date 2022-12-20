__int128 i;
#include <postgres.h>
#include <stddef.h>
#include <utils/memutils.h>

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

void gluepg_curl_init() {
  curl_global_init_mem(CURL_GLOBAL_DEFAULT, palloc, gluepg_curl_pgfree,
                       gluepg_curl_pgrealloc, pstrdup, gluepg_curl_pcalloc);
}

void gluepg_curl_buffer_init(gluepg_curl_buffer *buf) {
  buf->size = 0;
  buf->allocated = 0;
  buf->data = palloc(0);
}
void gluepg_curl_buffer_reset(gluepg_curl_buffer *buf) { buf->size = 0; }

size_t gluepg_curl_buffer_write(void *data, size_t size, size_t nmemb,
                                       void *buffer) {
  size_t realsize = size * nmemb;
  gluepg_curl_buffer *mem = (gluepg_curl_buffer *)buffer;

  uintptr_t new_size = mem->size + realsize + 1;
  if (mem->allocated < new_size) {
    mem->data = repalloc(mem->data, new_size);
    mem->allocated = new_size;
  }

  memcpy(&(mem->data[mem->size]), data, realsize);
  mem->size += realsize;
  mem->data[mem->size] = 0;

  return realsize;
}
