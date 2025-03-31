/**
 * @file omni_httpc.c
 *
 * Some portions of this code were liften from h2o's httpclient but are being changed over time
 * to address our specific needs better.
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/htup.h>
#include <catalog/pg_enum.h>
#include <executor/spi.h>
#include <funcapi.h>
#include <lib/stringinfo.h>
#include <miscadmin.h>
#include <utils/array.h>
#include <utils/builtins.h>

#include <h2o.h>

#include "ca-bundle.h"
#include "executor/executor.h"

#include "ssl.h"
#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/provider.h>
#endif

#include <utils/syscache.h>

#include "libpgaug.h"

PG_MODULE_MAGIC;

CACHED_OID(omni_http, http_header);
CACHED_OID(http_response);

#define IO_TIMEOUT 5000
#define MAX_REDIRECTS 5

//
// Global variables below are used to share information across requests
//

static h2o_httpclient_ctx_t ctx;
static h2o_multithread_queue_t *queue;
static h2o_multithread_receiver_t getaddr_receiver;

static h2o_httpclient_connection_pool_t *connpool;
static h2o_socketpool_t *sockpool;
static h2o_httpclient_head_cb on_connect(h2o_httpclient_t *client, const char *errstr,
                                         h2o_iovec_t *_method, h2o_url_t *url,
                                         const h2o_header_t **headers, size_t *num_headers,
                                         h2o_iovec_t *body,
                                         h2o_httpclient_proceed_req_cb *proceed_req_cb,
                                         h2o_httpclient_properties_t *props, h2o_url_t *origin);

static struct {
  ptls_iovec_t token;
  ptls_iovec_t ticket;
  quicly_transport_parameters_t tp;
} http3_session;

static int save_http3_ticket_cb(ptls_save_ticket_t *self, ptls_t *tls, ptls_iovec_t src) {
  quicly_conn_t *conn = *ptls_get_data_ptr(tls);
  assert(quicly_get_tls(conn) == tls);

  free(http3_session.ticket.base);
  http3_session.ticket = ptls_iovec_init(h2o_mem_alloc(src.len), src.len);
  memcpy(http3_session.ticket.base, src.base, src.len);
  http3_session.tp = *quicly_get_remote_transport_parameters(conn);
  return 0;
}

static ptls_save_ticket_t save_http3_ticket = {save_http3_ticket_cb};

static int save_http3_token_cb(quicly_save_resumption_token_t *self, quicly_conn_t *conn,
                               ptls_iovec_t token) {
  free(http3_session.token.base);
  http3_session.token = ptls_iovec_init(h2o_mem_alloc(token.len), token.len);
  memcpy(http3_session.token.base, token.base, token.len);
  return 0;
}

static quicly_save_resumption_token_t save_http3_token = {save_http3_token_cb};

static int load_http3_session(h2o_httpclient_ctx_t *ctx, struct sockaddr *server_addr,
                              const char *server_name, ptls_iovec_t *token, ptls_iovec_t *ticket,
                              quicly_transport_parameters_t *tp) {
  /* TODO respect server_addr, server_name */
  if (http3_session.token.base != NULL) {
    *token = ptls_iovec_init(h2o_mem_alloc(http3_session.token.len), http3_session.token.len);
    memcpy(token->base, http3_session.token.base, http3_session.token.len);
  }
  if (http3_session.ticket.base != NULL) {
    *ticket = ptls_iovec_init(h2o_mem_alloc(http3_session.ticket.len), http3_session.ticket.len);
    memcpy(ticket->base, http3_session.ticket.base, http3_session.ticket.len);
    *tp = http3_session.tp;
  }
  return 1;
}

static void init() {
  static bool initialized = false;
  if (initialized)
    return;

  static const ptls_key_exchange_algorithm_t *h3_key_exchanges[] = {
#if PTLS_OPENSSL_HAVE_X25519
    &ptls_openssl_x25519,
#endif
    &ptls_openssl_secp256r1,
    NULL
  };
  static h2o_http3client_ctx_t h3ctx = {
      .tls =
          {
              .random_bytes = ptls_openssl_random_bytes,
              .get_time = &ptls_get_time,
              .key_exchanges = h3_key_exchanges,
              .cipher_suites = ptls_openssl_cipher_suites,
              .save_ticket = &save_http3_ticket,
          },
  };

  ctx = (h2o_httpclient_ctx_t){
      .getaddr_receiver = &getaddr_receiver,
      .io_timeout = IO_TIMEOUT,
      .connect_timeout = IO_TIMEOUT,
      .first_byte_timeout = IO_TIMEOUT,
      .keepalive_timeout = IO_TIMEOUT,
      .max_buffer_size = H2O_SOCKET_INITIAL_INPUT_BUFFER_SIZE * 2,
      .http2 = {.max_concurrent_streams = 100},
      .http3 = &h3ctx,
  };

  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();

  /* When using OpenSSL >= 3.0, load legacy provider so that blowfish can be used for 64-bit QUIC
   * CIDs. */
#if OPENSSL_VERSION_MAJOR >= 3
  OSSL_PROVIDER_load(NULL, "legacy");
  OSSL_PROVIDER_load(NULL, "default");
#endif

  quicly_amend_ptls_context(&h3ctx.tls);
  h3ctx.quic = quicly_spec_context;
  h3ctx.quic.transport_params.max_streams_uni = 10;
  h3ctx.quic.transport_params.max_datagram_frame_size = 1500;
  h3ctx.quic.receive_datagram_frame = &h2o_httpclient_http3_on_receive_datagram_frame;
  h3ctx.quic.tls = &h3ctx.tls;
  h3ctx.quic.save_resumption_token = &save_http3_token;
  {
    uint8_t random_key[PTLS_SHA256_DIGEST_SIZE];
    h3ctx.tls.random_bytes(random_key, sizeof(random_key));
    h3ctx.quic.cid_encryptor = quicly_new_default_cid_encryptor(
        &ptls_openssl_bfecb, &ptls_openssl_aes128ecb, &ptls_openssl_sha256,
        ptls_iovec_init(random_key, sizeof(random_key)));
    assert(h3ctx.quic.cid_encryptor != NULL);
    ptls_clear_memory(random_key, sizeof(random_key));
  }
  h3ctx.quic.stream_open = &h2o_httpclient_http3_on_stream_open;
  h3ctx.load_session = load_http3_session;

  ctx.loop = h2o_evloop_create();
  {
    // initialize QUIC context
    int fd;
    struct sockaddr_in sin;
    if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
      ereport(ERROR, errmsg("failed to create UDP socket"));
    }
    memset(&sin, 0, sizeof(sin));
    if (bind(fd, (void *)&sin, sizeof(sin)) != 0) {
      ereport(ERROR, errmsg("failed to bind bind UDP socket"));
    }
    h2o_socket_t *sock = h2o_evloop_socket_create(ctx.loop, fd, H2O_SOCKET_FLAG_DONT_READ);
    h2o_quic_init_context(&h3ctx.h3, ctx.loop, sock, &h3ctx.quic, NULL, NULL,
                          h2o_httpclient_http3_notify_connection_update, 1, NULL);
  }

  queue = h2o_multithread_create_queue(ctx.loop);
  h2o_multithread_register_receiver(queue, ctx.getaddr_receiver, h2o_hostinfo_getaddr_receiver);

  connpool = h2o_mem_alloc(sizeof(*connpool));
  sockpool = h2o_mem_alloc(sizeof(*sockpool));

  // NB: It is very important to init the sockpool *prior* to
  // registering it in the loop as it will initialize its
  // content with zeroes.
  h2o_socketpool_init_global(sockpool, 1);

  h2o_socketpool_set_timeout(sockpool, IO_TIMEOUT);
  h2o_socketpool_register_loop(sockpool, ctx.loop);

  h2o_httpclient_connection_pool_init(connpool, sockpool);

  initialized = true;
}

// Contains request and its outcome
struct request {
  // Request body
  h2o_iovec_t request_body;
  // Response body
  StringInfoData body;
  // Request URL
  h2o_url_t url;
  // Error (NULL if none)
  const char *errstr;
  // Request method
  h2o_iovec_t method;
  // Request headers
  size_t num_headers;
  h2o_header_t *headers;
  // Response status
  int status;
  h2o_iovec_t status_message;
  // Response headers
  Datum *response_headers;
  size_t num_response_headers;
  // HTTP version
  int version;
  // Signals that the authority has been connected to for this request
  bool connected;
  // Signals that the request has finished processing
  bool complete;
  // follow HTTP redirects
  bool follow_redirects;
  // limit on the number of redirects to follow
  int redirects_left;
};

// Called when response body chunk is being received
static int on_body(h2o_httpclient_t *client, const char *errstr, h2o_header_t *trailers,
                   size_t num_trailers) {
  struct request *req = (struct request *)client->data;

  // If there's an error, report it
  if (errstr != NULL) {
    if (errstr != h2o_httpclient_error_is_eos) {
      req->errstr = errstr;
      req->complete = true;
      return -1;
    }
  }

  // Append the body if there's a body to consume
  h2o_buffer_t *buf = *client->buf;
  if (buf != NULL && buf->size > 0) {
    appendBinaryStringInfo(&req->body, buf->bytes, buf->size);
    // NB: here we use client->buf to ensure it is properly updated, if we pass
    // `&buf` then the actual buffer is not updated when it is fully consumed
    h2o_buffer_consume(client->buf, buf->size);
  }

  // End of stream, complete the request
  if (errstr == h2o_httpclient_error_is_eos) {
    SET_VARSIZE(req->body.data, req->body.len);
    req->complete = true;
  }

  return 0;
}

void copy_request_fields(struct request *src, struct request *dest) {
  dest->method = src->method;
  dest->request_body = src->request_body;
  dest->url = src->url;
  dest->headers = src->headers;
  dest->num_headers = src->num_headers;
  dest->status = src->status;
  dest->response_headers = src->response_headers;
  dest->num_response_headers = src->num_response_headers;
  dest->body = src->body;
  dest->version = src->version;
}

// Called when response head has been received
static h2o_httpclient_body_cb on_head(h2o_httpclient_t *client, const char *errstr,
                                      h2o_httpclient_on_head_t *args) {
  struct request *req = (struct request *)client->data;

  // If there's an error, report it
  if (errstr != NULL && errstr != h2o_httpclient_error_is_eos) {
    req->errstr = errstr;
    req->complete = true;
    return NULL;
  }

  bool handle_redirect =
      req->follow_redirects && req->redirects_left > 0 &&
      (args->status == 301 || args->status == 302 || args->status == 307 || args->status == 308);

  // Handle Redirect if necessary
  if (handle_redirect) {
    h2o_headers_t *response_headers_vec = (h2o_headers_t *)palloc0(sizeof(*response_headers_vec));
    response_headers_vec->entries = args->headers;
    response_headers_vec->size = args->num_headers;
    response_headers_vec->capacity = args->num_headers;

    ssize_t location_index;
    if ((location_index = h2o_find_header(response_headers_vec, H2O_TOKEN_LOCATION, -1)) > -1) {
      // create a new request based on the original request
      struct request *new_req = palloc0(sizeof(struct request));
      new_req->complete = false;
      new_req->connected = false;
      new_req->headers = req->headers;
      h2o_headers_t *request_headers_vec = (h2o_headers_t *)palloc0(sizeof(*request_headers_vec));
      request_headers_vec->entries = req->headers;
      request_headers_vec->size = req->num_headers;

      initStringInfo(&new_req->body);
      appendStringInfoSpaces(&new_req->body, sizeof(struct varlena));
      SET_VARSIZE(new_req->body.data, VARHDRSZ);

      new_req->num_headers = req->num_headers;
      new_req->errstr = NULL;
      new_req->version = req->version;
      new_req->follow_redirects = req->follow_redirects;
      req->status = args->status;
      req->status_message = args->msg;
      // for 307 and 308, we must preserve the method, even if non-idempotent
      if (args->status == 307 || args->status == 308) {
        new_req->method = req->method;
        new_req->request_body = req->request_body;
      } else {
        // omit the request body and content-length header
        int content_length_index =
            h2o_find_header(request_headers_vec, H2O_TOKEN_CONTENT_LENGTH, -1);
        if (content_length_index != -1) {
          h2o_delete_header(request_headers_vec, content_length_index);
        }
        new_req->request_body = h2o_iovec_init(NULL, 0);
        // if the method is POST, override the method as GET
        if (h2o_memis(req->method.base, req->method.len, H2O_STRLIT("POST"))) {
          new_req->method = h2o_iovec_init(H2O_STRLIT("GET"));
        } else {
          new_req->method = req->method;
        }
      }
      new_req->headers = request_headers_vec->entries;
      new_req->num_headers = request_headers_vec->size;

      h2o_url_t url;
      // re-evaluate location index after the headers have been modified
      location_index = h2o_find_header(response_headers_vec, H2O_TOKEN_LOCATION, -1);

      h2o_iovec_t location_header = response_headers_vec->entries[location_index].value;
      // try to parse the URL as an absolute URL
      int result = h2o_url_parse(client->pool, location_header.base, location_header.len, &url);
      if (result == -1) {
        // if the URL is not an absolute URL, treat it as a relative URL
        h2o_url_t relative_url;
        result = h2o_url_parse_relative(client->pool, location_header.base, location_header.len,
                                        &relative_url);
        if (result == -1) {
          ereport(ERROR, errmsg("location header value not a valid URL`"));
          return NULL;
        }
        // resolve the relative URL against the original URL
        h2o_url_resolve(client->pool, &req->url, &relative_url, &url);
      }
      // mark redirect as followed
      new_req->redirects_left = req->redirects_left - 1;
      new_req->url = url;

      // follow the URL in the location header and wait for the request to finish
      h2o_httpclient_connect(NULL, client->pool, new_req, client->ctx, client->connpool,
                             &new_req->url, NULL, on_connect);
      while (!new_req->complete) {
        CHECK_FOR_INTERRUPTS();
        h2o_evloop_run(ctx.loop, INT32_MAX);
      }
      // copy the response from the new request to the original request
      copy_request_fields(new_req, req);
      req->complete = true;
      return NULL;
    } else {
      ereport(
          WARNING,
          errmsg(
              "received a redirect-specific HTTP status %d without a corresponding location header",
              args->status));
    }
  }

  // Collect information from the head
  req->version = args->version;
  req->status = args->status;
  req->status_message = args->msg;

  req->num_response_headers = args->num_headers;
  req->response_headers = palloc(args->num_headers * sizeof(Datum));

  TupleDesc header_tupledesc = TypeGetTupleDesc(http_header_oid(), NULL);
  BlessTupleDesc(header_tupledesc);

  for (size_t i = 0; i < req->num_response_headers; i++) {
    HeapTuple header =
        heap_form_tuple(header_tupledesc,
                        (Datum[2]){PointerGetDatum(cstring_to_text_with_len(
                                       args->headers[i].name->base, args->headers[i].name->len)),
                                   PointerGetDatum(cstring_to_text_with_len(
                                       args->headers[i].value.base, args->headers[i].value.len))},
                        (bool[2]){false, false});
    req->response_headers[i] = HeapTupleGetDatum(header);
  }

  // End of stream, complete the request
  if (errstr == h2o_httpclient_error_is_eos) {
    req->complete = true;
    return NULL;
  }

  return on_body;
}

// Called when request is connected to the authority
static h2o_httpclient_head_cb on_connect(h2o_httpclient_t *client, const char *errstr,
                                         h2o_iovec_t *_method, h2o_url_t *url,
                                         const h2o_header_t **headers, size_t *num_headers,
                                         h2o_iovec_t *body,
                                         h2o_httpclient_proceed_req_cb *proceed_req_cb,
                                         h2o_httpclient_properties_t *props, h2o_url_t *origin) {
  struct request *req = (struct request *)client->data;
  req->connected = true;
  // If there's an error, report it
  if (errstr != NULL) {
    req->errstr = errstr;
    req->complete = true;
    return NULL;
  }
  // Provide H2O client with necessary request payload
  *_method = req->method;
  *num_headers = req->num_headers;
  *headers = req->headers;
  /*
  From https://h2o.examp1e.net/configure/proxy_directives.html#proxy.reverse.url

  "In addition to TCP/IP over IPv4 and IPv6, the proxy handler can also connect to an HTTP server
  listening to a Unix socket. Path to the unix socket should be surrounded by square brackets, and
  prefixed with unix: (e.g. http://[unix:/path/to/socket]/path)"
*/
  const char *uds_prefix = "[unix:";
  /*
  required to prevent malformed host header with unix domain sockets for eg. when calling
  http://[unix:/var/run/docker.sock]/_ping h2o sets the host header to [unix:/var/run/docker.sock]
  and golang disallows '/' in host header value with 400 Bad request
  */
  if (strncmp(req->url.authority.base, uds_prefix, strlen(uds_prefix)) == 0) {
    req->url.authority.len = 0;
  }
  *url = req->url;
  *body = req->request_body;
  *proceed_req_cb = NULL;
  return on_head;
}

int allow_self_signed_cert_cb(int preverify_ok, X509_STORE_CTX *ctx) {
  if (!preverify_ok) {
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
    int err = X509_STORE_CTX_get_error(ctx);

    // Allow self-signed certificates if this is the specific error
    if (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) {
      return 1;
    }
  }
  return preverify_ok;
}

PG_FUNCTION_INFO_V1(http_execute);

Datum http_execute(PG_FUNCTION_ARGS) {
  init();

  // Prepare to return the set
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  // Initialize a memory pool
  h2o_mem_pool_t *pool = (h2o_mem_pool_t *)palloc(sizeof(*pool));
  h2o_mem_init_pool(pool);

  bool follow_redirects = true;

  bool allow_self_signed_cert = false;
  ArrayType *cacerts = NULL;
  HeapTupleHeader clientcert = NULL;

  // Options
  {
    HeapTupleHeader options = PG_GETARG_HEAPTUPLEHEADER(0);
    bool isnull;

    // H2/H3 ratio
    Datum option_http2_ratio = GetAttributeByName(options, "http2_ratio", &isnull);
    int http2_ratio = 0;
    if (!isnull) {
      http2_ratio = DatumGetInt32(option_http2_ratio);
    }

    Datum option_follow_redirects = GetAttributeByName(options, "follow_redirects", &isnull);
    if (!isnull) {
      follow_redirects = DatumGetBool(option_follow_redirects);
    }

    Datum option_http3_ratio = GetAttributeByName(options, "http3_ratio", &isnull);
    int http3_ratio = 0;
    if (!isnull) {
      http3_ratio = DatumGetInt32(option_http3_ratio);
    }

    if (http2_ratio < 0 || http2_ratio > 100) {
      ereport(ERROR, errmsg("invalid option"), errdetail("http2_ratio"),
              errhint("must be within 0..100"));
    }

    if (http3_ratio < 0 || http3_ratio > 100) {
      ereport(ERROR, errmsg("invalid option"), errdetail("http3_ratio"),
              errhint("must be within 0..100"));
    }

    if (http2_ratio + http3_ratio > 100) {
      ereport(ERROR, errmsg("invalid option"), errdetail("http2_ratio+http3_ratio"),
              errhint("this sum must be within 0..100"));
    }

    ctx.protocol_selector.ratio.http2 = http2_ratio;
    ctx.protocol_selector.ratio.http3 = http3_ratio;

    // Clear text HTTP2
    Datum option_force_cleartext_http2 =
        GetAttributeByName(options, "force_cleartext_http2", &isnull);
    if (!isnull && DatumGetBool(option_force_cleartext_http2)) {
      ctx.force_cleartext_http2 = 1;
    }

    Datum option_first_byte_timeout = GetAttributeByName(options, "first_byte_timeout", &isnull);
    int first_byte_timeout = IO_TIMEOUT;
    if (!isnull) {
      first_byte_timeout = DatumGetInt32(option_first_byte_timeout);
    }
    ctx.first_byte_timeout = first_byte_timeout;

    Datum option_timeout = GetAttributeByName(options, "timeout", &isnull);
    int timeout = IO_TIMEOUT;
    if (!isnull) {
      timeout = DatumGetInt32(option_timeout);
    }
    ctx.connect_timeout = ctx.io_timeout = ctx.keepalive_timeout = timeout;

    Datum allow_self_signed_cert_datum =
        GetAttributeByName(options, "allow_self_signed_cert", &isnull);

    if (!isnull) {
      allow_self_signed_cert = DatumGetBool(allow_self_signed_cert_datum);
    }

    Datum cacerts_datum = GetAttributeByName(options, "cacerts", &isnull);

    if (!isnull) {
      cacerts = DatumGetArrayTypeP(cacerts_datum);
    }

    Datum clientcert_datum = GetAttributeByName(options, "clientcert", &isnull);

    if (!isnull) {
      clientcert = DatumGetHeapTupleHeader(clientcert_datum);
    }
  }

  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("can't have null requests"));
  }

  ArrayType *requests_array = PG_GETARG_ARRAYTYPE_P(1);
  ArrayIterator request_iter = array_create_iterator(requests_array, 0, NULL);

  int num_requests = ArrayGetNItems(ARR_NDIM(requests_array), ARR_DIMS(requests_array));
  struct request *requests = palloc_array(struct request, num_requests);

  bool any_requests = false;
  // For every request:
  for (int req_i = 0; req_i < num_requests; req_i++) {

    Datum request_datum;
    bool isnull;

    if (!array_iterate(request_iter, &request_datum, &isnull)) {
      // Bail if we're done iterating
      break;
    }

    // Skip NULL requests
    if (isnull) {
      continue;
    }
    any_requests = true;

    struct request *request = &requests[req_i];
    HeapTupleHeader arg_request = DatumGetHeapTupleHeader(request_datum);

    // Follow redirects?
    request->follow_redirects = follow_redirects;

    // URL
    Datum url = GetAttributeByName(arg_request, "url", &isnull);
    if (isnull) {
      ereport(ERROR, errmsg("URL can't be NULL"));
    }
    text *arg_url = (text *)PG_DETOAST_DATUM_PACKED(url);

    if (h2o_url_parse(pool, VARDATA_ANY(arg_url), VARSIZE_ANY_EXHDR(arg_url), &request->url) ==
        -1) {
      ereport(ERROR, errmsg("can't parse URL"), errdetail("%s", text_to_cstring(arg_url)));
    }

    // Method
    Oid arg_method = DatumGetObjectId(GetAttributeByName(arg_request, "method", &isnull));
    if (!isnull) {
      HeapTuple tup = SearchSysCache1(ENUMOID, arg_method);
      assert(HeapTupleIsValid(tup));

      Form_pg_enum enumval = (Form_pg_enum)GETSTRUCT(tup);

      request->method =
          h2o_iovec_init(pstrdup(enumval->enumlabel.data), strlen(enumval->enumlabel.data));
      ReleaseSysCache(tup);
    } else {
      request->method = h2o_iovec_init(H2O_STRLIT("GET"));
    }

    // Request body
    Datum arg_request_body = GetAttributeByName(arg_request, "body", &isnull);
    if (!isnull) {
      struct varlena *varlena = PG_DETOAST_DATUM_PACKED(arg_request_body);
      request->request_body = h2o_iovec_init(VARDATA_ANY(varlena), VARSIZE_ANY_EXHDR(varlena));
    } else {
      request->request_body = h2o_iovec_init(NULL, 0);
    }

    // Headers
    Datum arg_headers = GetAttributeByName(arg_request, "headers", &isnull);
    h2o_headers_t *headers_vec = (h2o_headers_t *)palloc0(sizeof(*headers_vec));

    if (!isnull) {
      TupleDesc tupdesc = TypeGetTupleDesc(http_header_oid(), NULL);
      BlessTupleDesc(tupdesc);

      ArrayIterator it = array_create_iterator(DatumGetArrayTypeP(arg_headers), 0, NULL);
      Datum value;
      while (array_iterate(it, &value, &isnull)) {
        if (!isnull) {
          HeapTupleHeader tuple = DatumGetHeapTupleHeader(value);
          Datum name = GetAttributeByNum(tuple, 1, &isnull);
          if (!isnull) {
            text *name_str = DatumGetTextPP(name);
            Datum value = GetAttributeByNum(tuple, 2, &isnull);
            if (!isnull) {
              text *value_str = DatumGetTextPP(value);
              h2o_add_header_by_str(pool, headers_vec, VARDATA_ANY(name_str),
                                    VARSIZE_ANY_EXHDR(name_str), 1, NULL, VARDATA_ANY(value_str),
                                    VARSIZE_ANY_EXHDR(value_str));
            } else {
              h2o_add_header_by_str(pool, headers_vec, VARDATA_ANY(name_str),
                                    VARSIZE_ANY_EXHDR(name_str), 1, NULL, "", 0);
            }
          }
        }
      }
      array_free_iterator(it);
    }

    // User agent
    h2o_add_header_by_str(pool, headers_vec, H2O_STRLIT("user-agent"), 1, NULL,
                          H2O_STRLIT("omni_httpc/" EXT_VERSION));

    // Ensure content length is set
    char clbuf[10];
    if (request->request_body.len > 0) {
      int clbuf_len = pg_ultoa_n(request->request_body.len, clbuf);
      h2o_add_header(pool, headers_vec, H2O_TOKEN_CONTENT_LENGTH, NULL, clbuf, clbuf_len);
    }

    request->redirects_left = MAX_REDIRECTS;
    request->errstr = NULL;
    request->num_headers = headers_vec->size;
    request->headers = headers_vec->entries;
    request->connected = false;
    request->complete = false;
    initStringInfo(&request->body);
    // Prepend the response with varlena's header so that we can populate it
    // in the end, when the ull length is known.
    appendStringInfoSpaces(&request->body, sizeof(struct varlena));
    SET_VARSIZE(request->body.data, VARHDRSZ);
  }
  array_free_iterator(request_iter);

  if (!any_requests) {
    Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
    rsinfo->setResult = tupstore;
#if PG_MAJORVERSION_NUM < 17
    tuplestore_donestoring(tupstore);
#endif
    goto done;
  }

  // Adjust the socket pool capacity if necessary
  sockpool->capacity = Max(num_requests, sockpool->capacity);

  // Load CA bundle if necessary
  if (sockpool->_ssl_ctx == NULL) {
    SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_client_method());
    int bundle_loaded = load_ca_bundle(ssl_ctx, ca_bundle);
    assert(bundle_loaded == 1);

    if (cacerts != NULL) {
      ArrayIterator it = array_create_iterator(cacerts, 0, NULL);
      Datum value;
      bool isnull;
      while (array_iterate(it, &value, &isnull)) {
        text *cert = DatumGetTextPP(value);
        BIO *cert_bio = BIO_new_mem_buf(VARDATA_ANY(cert), VARSIZE_ANY_EXHDR(cert));
        X509 *ca_cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);
        X509_STORE *store = SSL_CTX_get_cert_store(ssl_ctx);

        if (X509_STORE_add_cert(store, ca_cert) != 1) {
          ereport(ERROR, errmsg("error loading CA certificate"));
        };

        BIO_free(cert_bio);
        X509_free(ca_cert);
      }
      array_free_iterator(it);
    }

    if (clientcert != NULL) {
      bool isnull;
      Datum cert_datum = GetAttributeByName(clientcert, "certificate", &isnull);
      if (isnull) {
        ereport(ERROR, errmsg("client certificate must be supplied"));
      }
      Datum pkey_datum = GetAttributeByName(clientcert, "private_key", &isnull);
      if (isnull) {
        ereport(ERROR, errmsg("client private key must be supplied"));
      }

      text *cert = DatumGetTextPP(cert_datum);
      BIO *cert_bio = BIO_new_mem_buf(VARDATA_ANY(cert), VARSIZE_ANY_EXHDR(cert));
      X509 *client_cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);

      text *pkey = DatumGetTextPP(pkey_datum);
      BIO *pkey_bio = BIO_new_mem_buf(VARDATA_ANY(pkey), VARSIZE_ANY_EXHDR(pkey));
      EVP_PKEY *client_pkey = PEM_read_bio_PrivateKey(pkey_bio, NULL, NULL, NULL);

      SSL_CTX_use_certificate(ssl_ctx, client_cert);
      SSL_CTX_use_PrivateKey(ssl_ctx, client_pkey);

      BIO_free(cert_bio);
      X509_free(client_cert);

      BIO_free(pkey_bio);
      EVP_PKEY_free(client_pkey);
    }

    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       allow_self_signed_cert ? allow_self_signed_cert_cb : NULL);
    h2o_socketpool_set_ssl_ctx(sockpool, ssl_ctx);
    SSL_CTX_free(ssl_ctx);
  }

  // Send off all requests

  // Prevent the event loop from going stale and resulting in connection timeouts
  h2o_evloop_run(ctx.loop, 0);

  for (int i = 0; i < num_requests; i++) {
    struct request *request = &requests[i];
    h2o_httpclient_connect(NULL, pool, request, &ctx, connpool, &request->url, NULL, on_connect);

    // Wait until this request has been connected so that following requests can re-use the
    // connection
    while (!request->connected) {
      CHECK_FOR_INTERRUPTS();
      h2o_evloop_run(ctx.loop, INT32_MAX);
    }
  }

  // Run the event loop until all requests have been completed
  for (int i = 0; i < num_requests; i++) {
    struct request *request = &requests[i];
    while (!request->complete) {
      CHECK_FOR_INTERRUPTS();
      h2o_evloop_run(ctx.loop, INT32_MAX);
    }
  }

  // Start populating the result set
  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  // For every request
  for (int i = 0; i < num_requests; i++) {
    struct request *request = &requests[i];

    TupleDesc response_tupledesc = rsinfo->expectedDesc;

    if (request->errstr != NULL) {
      // Return an error if there's one
      bool isnull[6] = {[0 ... 3] = true, [4] = false, true};
      Datum values[6] = {
          0, 0, 0, 0, PointerGetDatum(cstring_to_text(request->errstr)), PointerGetDatum(NULL)};
      tuplestore_putvalues(tupstore, response_tupledesc, values, isnull);
    } else {
      // Return the response with no error
      ArrayType *response_headers = construct_md_array(
          request->response_headers, NULL, 1, (int[1]){request->num_response_headers}, (int[1]){1},
          http_header_oid(), -1, false, TYPALIGN_DOUBLE);

      Datum values[6] = {
          Int16GetDatum(request->version),
          Int16GetDatum(request->status),
          PointerGetDatum(response_headers),
          PointerGetDatum(request->body.data),
          PointerGetDatum(NULL),
          PointerGetDatum(request->status_message.len == 0
                              ? NULL
                              : cstring_to_text_with_len(request->status_message.base,
                                                         request->status_message.len))};
      bool isnull[6] = {[0 ... 3] = false, [4] = true, [5] = request->status_message.len == 0};
      tuplestore_putvalues(tupstore, response_tupledesc, values, isnull);
    }
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

done:
  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(http_connections);

Datum http_connections(PG_FUNCTION_ARGS) {
  init();

  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  // Scan H2 connections
  for (h2o_linklist_t *l = connpool->http2.conns.next; l != &connpool->http2.conns; l = l->next) {
    struct st_h2o_httpclient__h2_conn_t *conn =
        H2O_STRUCT_FROM_MEMBER(struct st_h2o_httpclient__h2_conn_t, link, l);

    text *url =
        cstring_to_text_with_len(conn->origin_url.authority.base, conn->origin_url.authority.len);
    Datum values[2] = {Int16GetDatum(2), PointerGetDatum(url)};
    bool isnull[2] = {false, false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

  // Scan H3 connections
  for (h2o_linklist_t *l = connpool->http3.conns.next; l != &connpool->http3.conns; l = l->next) {
    struct st_h2o_httpclient__h3_conn_t *conn =
        H2O_STRUCT_FROM_MEMBER(struct st_h2o_httpclient__h3_conn_t, link, l);

    text *url = cstring_to_text_with_len(conn->server.origin_url.authority.base,
                                         conn->server.origin_url.authority.len);
    Datum values[2] = {Int16GetDatum(3), PointerGetDatum(url)};
    bool isnull[2] = {false, false};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

#if PG_MAJORVERSION_NUM < 17
  tuplestore_donestoring(tupstore);
#endif

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}