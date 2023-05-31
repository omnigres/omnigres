#ifndef OMNI_HTTPC_SSL_H
#define OMNI_HTTPC_SSL_H

#include <openssl/ssl.h>

int load_ca_bundle(SSL_CTX *ctx, const char *cert_str);

#endif // OMNI_HTTPC_SSL_H