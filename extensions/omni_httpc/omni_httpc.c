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

static h2o_httpclient_ctx_t ctx;
static h2o_multithread_queue_t *queue;
static h2o_multithread_receiver_t getaddr_receiver;

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
      perror("failed to create UDP socket");
      exit(EXIT_FAILURE);
    }
    memset(&sin, 0, sizeof(sin));
    if (bind(fd, (void *)&sin, sizeof(sin)) != 0) {
      perror("failed to bind bind UDP socket");
      exit(EXIT_FAILURE);
    }
    h2o_socket_t *sock = h2o_evloop_socket_create(ctx.loop, fd, H2O_SOCKET_FLAG_DONT_READ);
    h2o_quic_init_context(&h3ctx.h3, ctx.loop, sock, &h3ctx.quic, NULL,
                          h2o_httpclient_http3_notify_connection_update, 1, NULL);
  }

  queue = h2o_multithread_create_queue(ctx.loop);
  h2o_multithread_register_receiver(queue, ctx.getaddr_receiver, h2o_hostinfo_getaddr_receiver);

  initialized = true;
}

struct request {
  h2o_iovec_t request_body;
  StringInfoData body;
  bool done;
  h2o_url_t url;
  const char *errstr;
  h2o_iovec_t method;
  size_t num_headers;
  h2o_header_t **headers;
  int status;
  Datum *response_headers;
  size_t num_response_headers;
};

static int on_body(h2o_httpclient_t *client, const char *errstr) {
  struct request *req = (struct request *)client->data;

  if (errstr != NULL) {
    if (errstr != h2o_httpclient_error_is_eos) {
      req->errstr = errstr;
      req->done = true;
      return -1;
    }
  }
  appendBinaryStringInfo(&req->body, (*client->buf)->bytes, (*client->buf)->size);
  h2o_buffer_consume(&(*client->buf), (*client->buf)->size);

  if (errstr == h2o_httpclient_error_is_eos) {
    h2o_mem_clear_pool(client->pool);
    SET_VARSIZE(req->body.data, req->body.len);
    req->done = true;
  }

  return 0;
}

static h2o_httpclient_body_cb on_head(h2o_httpclient_t *client, const char *errstr,
                                      h2o_httpclient_on_head_t *args) {
  struct request *req = (struct request *)client->data;

  if (errstr != NULL && errstr != h2o_httpclient_error_is_eos) {
    req->errstr = errstr;
    req->done = true;
    return NULL;
  }

  if (errstr == h2o_httpclient_error_is_eos) {
    req->done = true;
    return NULL;
  }

  req->status = args->status;

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
  return on_body;
}

static h2o_httpclient_head_cb on_connect(h2o_httpclient_t *client, const char *errstr,
                                         h2o_iovec_t *_method, h2o_url_t *url,
                                         const h2o_header_t **headers, size_t *num_headers,
                                         h2o_iovec_t *body,
                                         h2o_httpclient_proceed_req_cb *proceed_req_cb,
                                         h2o_httpclient_properties_t *props, h2o_url_t *origin) {
  struct request *req = (struct request *)client->data;
  if (errstr != NULL) {
    req->errstr = errstr;
    req->done = true;
    return NULL;
  }
  *_method = req->method;
  *num_headers = req->num_headers;
  *headers = (h2o_header_t *)req->headers;
  *url = req->url;
  *body = req->request_body;
  *proceed_req_cb = NULL;
  return on_head;
}

PG_FUNCTION_INFO_V1(http_execute);

Datum http_execute(PG_FUNCTION_ARGS) {
  init();

  h2o_mem_pool_t *pool = (h2o_mem_pool_t *)palloc(sizeof(*pool));
  h2o_mem_init_pool(pool);

  struct request *request = palloc(sizeof(struct request));

  HeapTupleHeader arg_request = PG_GETARG_HEAPTUPLEHEADER(0);
  bool isnull = false;

  // URL
  text *arg_url = (text *)PG_DETOAST_DATUM_PACKED(GetAttributeByName(arg_request, "url", &isnull));

  if (isnull) {
    ereport(ERROR, errmsg("URL can't be NULL"));
  }

  h2o_url_t url;
  if (h2o_url_parse(VARDATA_ANY(arg_url), VARSIZE_ANY_EXHDR(arg_url), &url) == -1) {
    ereport(ERROR, errmsg("can't parse URL"), errdetail("%s", text_to_cstring(arg_url)));
  }

  // Method
  h2o_iovec_t method = h2o_iovec_init(H2O_STRLIT("GET"));
  Oid arg_method = DatumGetObjectId(GetAttributeByName(arg_request, "method", &isnull));
  if (!isnull) {
    HeapTuple tup = SearchSysCache1(ENUMOID, arg_method);
    assert(HeapTupleIsValid(tup));

    Form_pg_enum enumval = (Form_pg_enum)GETSTRUCT(tup);

    method = h2o_iovec_init(enumval->enumlabel.data, strlen(enumval->enumlabel.data));
    ReleaseSysCache(tup);
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
  static h2o_iovec_t user_agent_header_name = (h2o_iovec_t){H2O_STRLIT("user-agent")};
  h2o_header_t **headers;
  size_t num_headers = 0;
  h2o_headers_t headers_vec = {NULL};

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
          Datum value = GetAttributeByNum(tuple, 1, &isnull);
          if (!isnull) {
            text *value_str = DatumGetTextPP(value);
            h2o_add_header_by_str(pool, &headers_vec, VARDATA_ANY(name_str),
                                  VARSIZE_ANY_EXHDR(name_str), 1, NULL, VARDATA_ANY(value_str),
                                  VARSIZE_ANY_EXHDR(value_str));
          }
        }
      }
    }
    array_free_iterator(it);
  }

  h2o_add_header_by_str(pool, &headers_vec, H2O_STRLIT("user-agent"), 1, NULL,
                        H2O_STRLIT("omni_httpc/" EXT_VERSION));

  // Ensure content length is set
  char clbuf[10];
  if (request->request_body.len > 0) {
    int clbuf_len = pg_ultoa_n(request->request_body.len, clbuf);
    h2o_add_header(pool, &headers_vec, H2O_TOKEN_CONTENT_LENGTH, NULL, clbuf, clbuf_len);
  }

  headers = (h2o_header_t **)headers_vec.entries;
  num_headers = headers_vec.size;

  h2o_httpclient_connection_pool_t *connpool;

  connpool = palloc(sizeof(*connpool));
  h2o_socketpool_t *sockpool = h2o_mem_alloc(sizeof(*sockpool));
  h2o_socketpool_target_t *target = h2o_socketpool_create_target(&url, NULL);
  h2o_socketpool_init_specific(sockpool, 1, &target, 1, NULL);
  h2o_socketpool_set_timeout(sockpool, IO_TIMEOUT);
  h2o_socketpool_register_loop(sockpool, ctx.loop);
  h2o_httpclient_connection_pool_init(connpool, sockpool);

  SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_client_method());
  assert(load_ca_bundle(ssl_ctx, ca_bundle) == 1);
  SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
  h2o_socketpool_set_ssl_ctx(sockpool, ssl_ctx);
  SSL_CTX_free(ssl_ctx);

  request->done = false;
  request->url = url;
  request->method = method;
  request->errstr = NULL;
  request->num_headers = num_headers;
  request->headers = headers;
  initStringInfo(&request->body);
  appendStringInfoSpaces(&request->body, sizeof(struct varlena));
  SET_VARSIZE(request->body.data, VARHDRSZ);

  h2o_httpclient_connect(NULL, pool, request, &ctx, connpool, &url, NULL, on_connect);

  while (!request->done) {
    CHECK_FOR_INTERRUPTS();
    h2o_evloop_run(ctx.loop, 0);
  }

  h2o_socketpool_unregister_loop(sockpool, ctx.loop);

  if (request->errstr != NULL) {
    ereport(ERROR, errmsg("%s", request->errstr),
            errdetail("url: %.*s", (int)VARSIZE_ANY_EXHDR(arg_url), VARDATA_ANY(arg_url)));
  }

  TupleDesc response_tupledesc = TypeGetTupleDesc(http_response_oid(), NULL);
  BlessTupleDesc(response_tupledesc);

  ArrayType *response_headers = construct_md_array(
      request->response_headers, NULL, 1, (int[1]){request->num_response_headers}, (int[1]){1},
      http_header_oid(), -1, false, TYPALIGN_DOUBLE);

  Datum values[3] = {Int16GetDatum(request->status), PointerGetDatum(response_headers),
                     PointerGetDatum(request->body.data)};

  HeapTuple response = heap_form_tuple(response_tupledesc, values, (bool[3]){false, false, false});

  PG_RETURN_DATUM(HeapTupleGetDatum(response));
}