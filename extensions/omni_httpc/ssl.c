#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

int load_ca_bundle(SSL_CTX *ctx, const char *cert_str) {
  BIO *bio_cert = NULL;
  int result = 0;

  // Create a BIO for the certificate string
  bio_cert = BIO_new_mem_buf(cert_str, -1);
  if (!bio_cert) {
    goto cleanup;
  }

  X509_STORE *store = SSL_CTX_get_cert_store(ctx);

  // Load certificates from the BIO and add them to the SSL_CTX stor
  X509 *cert;
  while ((cert = PEM_read_bio_X509(bio_cert, NULL, NULL, NULL)) != NULL) {
    int rc = X509_STORE_add_cert(store, cert);
    X509_free(cert);
    if (rc == 0) {
      goto cleanup;
    }
  }

  result = 1;

cleanup:
  if (bio_cert) {
    BIO_free(bio_cert);
  }

  return result;
}