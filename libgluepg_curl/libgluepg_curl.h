#ifndef LIBGLUEPG_CURL_H
#define LIBGLUEPG_CURL_H

#include <curl/curl.h>

/**
 * Asserts valid execution of a CURL call, reports the error to Postgres
 * otherwise
 *
 * This is done as a macro to ensure `errfinish()` gets proper file, line and
 * function information.
 *
 */
#define CURL_Assert(call)                                                      \
  {                                                                            \
    CURLcode code = call;                                                      \
    if (code != CURLE_OK)                                                      \
      ereport(ERROR, errmsg("CURL error"),                                     \
              errdetail("%s", curl_easy_strerror(code)));                      \
  }

#undef curl_easy_setopt
/**
 * Set CURL option, asserting correct return code
 */
#define curl_easy_setopt(handle, opt, value)                                   \
  CURL_Assert(curl_easy_setopt(handle, opt, value))

/**
 * Perform CURL operation, asserting correct return code
 */
#define curl_easy_perform(handle) CURL_Assert(curl_easy_perform(handle))

void *gluepg_curl_pcalloc(uintptr_t num, uintptr_t count);
void gluepg_curl_pgfree(void *ptr);
void *gluepg_curl_pgrealloc(void *ptr, uintptr_t size);
void gluepg_curl_init();

typedef struct {
  char *data;
  uintptr_t size;
  uintptr_t allocated;
} gluepg_curl_buffer;

void gluepg_curl_buffer_init(gluepg_curl_buffer *buf);
void gluepg_curl_buffer_reset(gluepg_curl_buffer *buf);

size_t gluepg_curl_buffer_write(void *data, size_t size, size_t nmemb,
                                void *buffer);

#endif // LIBGLUEPG_CURL_H
