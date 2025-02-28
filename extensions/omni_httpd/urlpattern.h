#ifndef OMNI_HTTPD_URLPATTERN_H
#define OMNI_HTTPD_URLPATTERN_H

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

typedef struct {
  char *protocol;
  size_t protocol_len;
  char *username;
  size_t username_len;
  char *password;
  size_t password_len;
  char *hostname;
  size_t hostname_len;
  int port;
  char *pathname;
  size_t pathname_len;
  char *search;
  size_t search_len;
  char *hash;
  size_t hash_len;
} omni_httpd_urlpattern_t;

bool match_urlpattern(omni_httpd_urlpattern_t *pat, char *input_data, size_t input_size);

#endif // OMNI_HTTPD_URLPATTERN_H
