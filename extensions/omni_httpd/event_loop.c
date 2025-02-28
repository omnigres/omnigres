#include <signal.h>
#include <stdatomic.h>

#include <h2o.h>
#include <h2o/websocket.h>

#include "event_loop.h"

#ifdef POSTGRES_H
#error "event_loop.c can't use any Postgres API"
#endif

h2o_evloop_t *worker_event_loop;

atomic_bool worker_running;
atomic_bool worker_reload;
bool event_loop_suspended;
bool event_loop_resumed;
pthread_mutex_t event_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t event_loop_resume_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t event_loop_resume_cond_ack = PTHREAD_COND_INITIALIZER;

h2o_multithread_receiver_t event_loop_receiver;
h2o_multithread_queue_t *event_loop_queue;

static pid_t MyPid;

// Defines cset_socket
#define i_val h2o_socket_t *
#define i_tag socket
#include <stc/cset.h>

cset_socket paused_listeners;

static size_t requests_in_flight = 0;

typedef enum {
  MSG_KIND_SEND,
  MSG_KIND_ABORT,
  MSG_KIND_PROXY,
  MSG_KIND_UPGRADE_TO_WEBSOCKET
} send_message_kind;

typedef struct {
  h2o_multithread_message_t super;
  request_message_t *reqmsg;
  send_message_kind kind;
  union {
    struct send_payload {
      size_t bufcnt;
      h2o_send_state_t state;
      h2o_sendvec_t vecs;
    } send;
    struct {
      char *url;
      bool preserve_host;
    } proxy;
    struct {
      void *uuid;
    } upgrade_to_websocket;
  } payload;
} send_message_t;

static void h2o_queue_send(request_message_t *msg, h2o_iovec_t *bufs, size_t bufcnt,
                           h2o_send_state_t state) {

  h2o_req_t *req = msg->req;
  if (req == NULL) {
    // The connection is gone, bail
    return;
  }

  send_message_t *message = malloc(sizeof(*message) - (sizeof(h2o_sendvec_t) * (bufcnt - 1)));
  size_t i;
  message->reqmsg = msg;
  message->kind = MSG_KIND_SEND;
  message->payload.send.bufcnt = bufcnt;
  message->payload.send.state = state;

  for (i = 0; i != bufcnt; ++i)
    h2o_sendvec_init_raw(&message->payload.send.vecs + i, bufs[i].base, bufs[i].len);

  message->super = (h2o_multithread_message_t){{NULL}};

  h2o_multithread_send_message(&event_loop_receiver, &message->super);
}

void h2o_queue_send_inline(request_message_t *msg, const char *body, size_t len) {
  h2o_req_t *req = msg->req;
  if (req == NULL) {
    // The connection is gone, bail
    return;
  }
  static h2o_generator_t generator = {NULL, NULL};

  h2o_iovec_t buf = {.base = (char *)body, .len = len};

  /* the function intentionally does not set the content length, since it may be used for generating
   * 304 response, etc. */
  /* req->res.content_length = buf.len; */

  h2o_start_response(req, &generator);

  if (h2o_memis(req->input.method.base, req->input.method.len, H2O_STRLIT("HEAD")))
    h2o_queue_send(msg, NULL, 0, H2O_SEND_STATE_FINAL);
  else
    h2o_queue_send(msg, &buf, 1, H2O_SEND_STATE_FINAL);
}

void h2o_queue_abort(request_message_t *msg) {
  h2o_req_t *req = msg->req;
  if (req == NULL) {
    // The connection is gone, bail
    return;
  }

  send_message_t *message = malloc(sizeof(*message));
  message->reqmsg = msg;
  message->kind = MSG_KIND_ABORT;

  message->super = (h2o_multithread_message_t){{NULL}};

  h2o_multithread_send_message(&event_loop_receiver, &message->super);
}

void h2o_queue_proxy(request_message_t *msg, char *url, bool preserve_host) {
  h2o_req_t *req = msg->req;
  if (req == NULL) {
    // The connection is gone, bail
    return;
  }

  send_message_t *message = malloc(sizeof(*message));
  message->reqmsg = msg;
  message->kind = MSG_KIND_PROXY;
  message->payload.proxy.url = url;
  message->payload.proxy.preserve_host = preserve_host;

  message->super = (h2o_multithread_message_t){{NULL}};

  h2o_multithread_send_message(&event_loop_receiver, &message->super);
}

void h2o_queue_upgrade_to_websocket(request_message_t *msg) {
  h2o_req_t *req = msg->req;
  if (req == NULL) {
    // The connection is gone, bail
    return;
  }

  send_message_t *message = malloc(sizeof(*message));
  message->reqmsg = msg;
  message->kind = MSG_KIND_UPGRADE_TO_WEBSOCKET;

  message->super = (h2o_multithread_message_t){{NULL}};

  h2o_multithread_send_message(&event_loop_receiver, &message->super);
}

static inline void prepare_req_for_reprocess(h2o_req_t *req) {
  req->conn->ctx->proxy.client_ctx.tunnel_enabled = 1;

  // Ensure we attempt H2
  req->conn->ctx->proxy.client_ctx.protocol_selector.ratio.http2 = 100;

  // This line below setting max_buffer_size is currently very important
  // because
  // https://github.com/h2o/h2o/blob/b5dca420963928865b28538a315f16cac5ae8251/lib/common/http1client.c#L881
  // will stop reading from the socket if max_buffer_size == 0 and max_buffer_size == 0
  // as 0 >= 0:
  // https://github.com/h2o/h2o/issues/3225
  req->conn->ctx->proxy.client_ctx.max_buffer_size = H2O_SOCKET_INITIAL_INPUT_BUFFER_SIZE * 2;
}

typedef struct on_ws_message_data {
  websocket_uuid_t ws_uuid;
  int unix_socket;
} on_ws_message_data_t;

static void on_ws_message(h2o_websocket_conn_t *conn,
                          const struct wslay_event_on_msg_recv_arg *arg) {
  on_ws_message_data_t *data = (on_ws_message_data_t *)conn->data;

  if (data != NULL && arg == NULL) {
    handler_message_t *msg = malloc(sizeof(*msg));
    msg->super = (h2o_multithread_message_t){{NULL}};
    msg->type = handler_message_websocket_close;
    memcpy(msg->payload.websocket_close.uuid, data->ws_uuid,
           sizeof(msg->payload.websocket_close.uuid));

    h2o_multithread_send_message(&handler_receiver, &msg->super);

    // Close & remove the socket
    close(data->unix_socket);
    unlink_websocket_unix_socket(&data->ws_uuid);

    // Mark this connection as closed and dispose the UUID
    h2o_websocket_close(conn);

    free(conn->data);
    conn->data = NULL;
    return;
  }

  if (arg != NULL && !wslay_is_ctrl_frame(arg->opcode)) {
    handler_message_t *msg = malloc(sizeof(*msg));
    msg->super = (h2o_multithread_message_t){{NULL}};
    msg->type = handler_message_websocket_message;
    msg->payload.websocket_message.opcode = arg->opcode;
    msg->payload.websocket_message.message = (uint8_t *)malloc(arg->msg_length);
    msg->payload.websocket_message.message_len = arg->msg_length;
    memcpy(msg->payload.websocket_message.message, arg->msg, arg->msg_length);
    memcpy(msg->payload.websocket_message.uuid, data->ws_uuid,
           sizeof(msg->payload.websocket_open.uuid));

    h2o_multithread_send_message(&handler_receiver, &msg->super);
  }
}

// Taken from h2o:
// https://github.com/h2o/h2o/blob/c54c63285b52421da2782f028022647fc2ea3dd1/lib/common/rand.c#L46-L86
// We are depending on it anyway, but this function is declared static there.
static void format_uuid_rfc4122(char *dst, uint8_t *octets, uint8_t version) {
  // Variant:
  // > Set the two most significant bits (bits 6 and 7) of the
  // > clock_seq_hi_and_reserved to zero and one, respectively.
  octets[8] = (octets[8] & 0x3f) | 0x80;
  // Version:
  // > Set the four most significant bits (bits 12 through 15) of the
  // > time_hi_and_version field to the 4-bit version number from
  // > Section 4.1.3.
  octets[6] = (octets[6] & 0x0f) | (version << 4);

  // String Representation:
  // > UUID  = time-low "-" time-mid "-"
  // >         time-high-and-version "-"
  // >         clock-seq-and-reserved
  // >         clock-seq-low "-" node
  // See also "4.1.2. Layout and Byte Order" for the layout
  size_t pos = 0;

#define UUID_ENC_PART(first, last)                                                                 \
  do {                                                                                             \
    h2o_hex_encode(&dst[pos], &octets[first], last - first + 1);                                   \
    pos += (last - first + 1) * 2;                                                                 \
  } while (0)

  UUID_ENC_PART(0, 3); /* time_low */
  dst[pos++] = '-';
  UUID_ENC_PART(4, 5); /* time_mid */
  dst[pos++] = '-';
  UUID_ENC_PART(6, 7); /* time_hi_and_version */
  dst[pos++] = '-';
  UUID_ENC_PART(8, 8); /* clock_seq_hi_and_reserved */
  UUID_ENC_PART(9, 9); /* clock_seq_low */
  dst[pos++] = '-';
  UUID_ENC_PART(10, 15); /* node */

#undef UUID_ENC_PART

  /* '\0' is set by h2o_hex_encode() */
}

int websocket_unix_socket_path(struct sockaddr_un *addr, websocket_uuid_t *uuid) {
#define SOCK_PREFIX "/omni_httpd.sock."
#define RFC4122_UUID_LEN 36

  extern char **temp_dir; // from omni_httpd
  const int len_with_sock_prefix = strlen(*temp_dir) + sizeof(SOCK_PREFIX) - 1;

  if (len_with_sock_prefix + RFC4122_UUID_LEN + 1 > sizeof(addr->sun_path))
    return -EINVAL;

  snprintf(addr->sun_path, sizeof(addr->sun_path), "%s" SOCK_PREFIX, *temp_dir);

  format_uuid_rfc4122(addr->sun_path + len_with_sock_prefix, *uuid, 4);

  return 0;
}

int unlink_websocket_unix_socket(websocket_uuid_t *uuid) {
  struct sockaddr_un addr = {0};

  if (websocket_unix_socket_path(&addr, uuid) < 0)
    return -EINVAL;

  return unlink(addr.sun_path);
}

static void on_message(h2o_multithread_receiver_t *receiver, h2o_linklist_t *messages) {
  while (!h2o_linklist_is_empty(messages)) {
    h2o_multithread_message_t *message =
        H2O_STRUCT_FROM_MEMBER(h2o_multithread_message_t, link, messages->next);
    send_message_t *send_msg = (send_message_t *)messages->next;
    request_message_t *reqmsg = send_msg->reqmsg;
    pthread_mutex_lock(&reqmsg->mutex);
    if (--requests_in_flight == 0) {
      c_FOREACH(it, cset_socket, paused_listeners) { h2o_socket_read_start(*it.ref, on_accept); }
      cset_socket_clear(&paused_listeners);
    }

    if (reqmsg->req == NULL) {
      // Connection is gone, bail
      // Can release request message
      free(reqmsg);
      goto done;
    }
    h2o_req_t *req = reqmsg->req;
    switch (send_msg->kind) {
    case MSG_KIND_SEND:
      h2o_sendvec(req, &send_msg->payload.send.vecs, send_msg->payload.send.bufcnt,
                  send_msg->payload.send.state);
      break;
    case MSG_KIND_ABORT:
      // Abort requested, however I am not currently sure what's the best way
      // to do this with libh2o, so just sending an empty response. (TODO/FIXME)
      req->res.status = 201;
      req->res.content_length = 0;
      h2o_send_inline(req, NULL, 0);
      break;
    case MSG_KIND_PROXY: {
      __auto_type proxy = send_msg->payload.proxy;
      h2o_req_overrides_t *overrides = h2o_mem_alloc_pool(&req->pool, h2o_req_overrides_t, 1);

      *overrides = (h2o_req_overrides_t){NULL};

      overrides->use_proxy_protocol = false;
      overrides->proxy_preserve_host = proxy.preserve_host;
      overrides->forward_close_connection = true;
      overrides->upstream = (h2o_url_t *)h2o_mem_alloc_pool(&req->pool, h2o_url_t, 1);

      h2o_url_parse(&req->pool, proxy.url, strlen(proxy.url), overrides->upstream);

      prepare_req_for_reprocess(req);

      h2o_reprocess_request(req, req->method, overrides->upstream->scheme,
                            overrides->upstream->authority, overrides->upstream->path, overrides,
                            0);
      break;
    }
    case MSG_KIND_UPGRADE_TO_WEBSOCKET: {
      on_ws_message_data_t *data = malloc(sizeof(on_ws_message_data_t));
      if (!data) {
        perror("malloc on_ws_message_data_t");
        goto done;
      }

      memcpy(data->ws_uuid, reqmsg->ws_uuid, sizeof(reqmsg->ws_uuid));

      // Create and listen on a Unix domain socket
      struct sockaddr_un server_addr = {.sun_family = AF_UNIX};

      if (websocket_unix_socket_path(&server_addr, &reqmsg->ws_uuid) < 0 ||
          (data->unix_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("creating unix listen socket");
        free(data);
        goto done;
      }

      if ((bind(data->unix_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) ||
          (listen(data->unix_socket, SOMAXCONN) < 0)) {
        perror("bind listen unix socket");
        close(data->unix_socket);
        free(data);
        goto done;
      }

      h2o_websocket_conn_t *websocket_conn =
          h2o_upgrade_to_websocket(req, reqmsg->ws_client_key, data, on_ws_message);
      assert(websocket_conn->data == data);

      h2o_socket_t *sock =
          h2o_evloop_socket_create(worker_event_loop, data->unix_socket, H2O_SOCKET_FLAG_DONT_READ);
      sock->data = websocket_conn;

      h2o_socket_read_start(sock, on_accept_ws_unix);

      handler_message_t *msg = malloc(sizeof(*msg));
      msg->super = (h2o_multithread_message_t){{NULL}};
      msg->type = handler_message_websocket_open;
      memcpy(msg->payload.websocket_open.uuid, reqmsg->ws_uuid,
             sizeof(msg->payload.websocket_open.uuid));

      h2o_multithread_send_message(&handler_receiver, &msg->super);

      goto done;
    }
    }
  done:
    pthread_mutex_unlock(&reqmsg->mutex);
    h2o_linklist_unlink(&message->link);
    free(send_msg);
  }
}

void event_loop_register_receiver() {
  h2o_multithread_register_receiver(event_loop_queue, &event_loop_receiver, on_message);
}

void *event_loop(void *arg) {
  assert(worker_event_loop != NULL);
  assert(handler_queue != NULL);
  assert(event_loop_queue != NULL);

  MyPid = getpid();

  paused_listeners = cset_socket_init();

  bool running = atomic_load(&worker_running);
  bool reload = atomic_load(&worker_reload);

  while (running) {
    if (event_loop_suspended) {
      pthread_mutex_lock(&event_loop_mutex);
      while (event_loop_suspended) {
        pthread_cond_wait(&event_loop_resume_cond, &event_loop_mutex);
      }
      event_loop_resumed = true;
      pthread_cond_signal(&event_loop_resume_cond_ack);
      pthread_mutex_unlock(&event_loop_mutex);
    }

    running = true;
    reload = false;

    while (running && !reload) {
      while ((running = atomic_load(&worker_running)) && !(reload = atomic_load(&worker_reload)) &&
             h2o_evloop_run(worker_event_loop, INT32_MAX) == 0)
        ;
    }

    {
      // Ensure we don't have any paused listeners as listeners may be changed upon reload
      // and sockets may be deallocated. It would be extremely unsafe to do it later as these
      // records may be gone.
      // The idea is that if they'll be resumed now, if the system is still busy, they'll
      // get stopped again.
      c_FOREACH(it, cset_socket, paused_listeners) { h2o_socket_read_start(*it.ref, on_accept); }
      cset_socket_clear(&paused_listeners);
    }

    // Make sure request handler no longer waits
    pthread_mutex_lock(&event_loop_mutex);
    event_loop_resumed = false;
    event_loop_suspended = true;
    pthread_cond_signal(&event_loop_resume_cond_ack);
    pthread_mutex_unlock(&event_loop_mutex);
  }
  return NULL;
}

void on_accept(h2o_socket_t *listener, const char *err) {
  if (requests_in_flight > 0) {
    // Don't accept new connections if this instance is busy as we'd likely
    // have to proxy it (or put in the queue if it is not HTTP/2+)
    h2o_socket_read_stop(listener);
    cset_socket_push(&paused_listeners, listener);
    return;
  }
  h2o_socket_t *sock;

  if (err != NULL) {
    return;
  }

  if ((sock = h2o_evloop_socket_accept(listener)) == NULL) {
    perror("accept http listener");
    return;
  }
  h2o_accept(listener_accept_ctx(listener), sock);
}

static void on_ws_relay_message(h2o_socket_t *sock, const char *err) {
  if (err != NULL) {
    h2o_socket_close(sock);
    return;
  }
  h2o_websocket_conn_t *conn = (h2o_websocket_conn_t *)sock->data;

  while (sock->input->size > 0) {
    struct {
      int8_t kind;
      size_t length;
    } __attribute__((packed)) *hdr = (typeof(hdr))sock->input->bytes;

    if (sock->input->size < sizeof(*hdr)) {
      break;
    }

    if (sock->input->size < sizeof(*hdr) + hdr->length) {
      break;
    }

    void *data = sock->input->bytes + sizeof(*hdr);

    const struct wslay_event_msg arg = {.opcode =
                                            hdr->kind == 0 ? WSLAY_TEXT_FRAME : WSLAY_BINARY_FRAME,
                                        .msg = data,
                                        .msg_length = hdr->length};
    wslay_event_queue_msg(conn->ws_ctx, &arg);

    h2o_buffer_consume(&sock->input, sizeof(*hdr) + hdr->length);
  }

  h2o_websocket_proceed(conn);

  h2o_socket_read_start(sock, on_ws_relay_message);
}

void on_accept_ws_unix(h2o_socket_t *listener, const char *err) {
  h2o_socket_t *sock;

  if (err != NULL) {
    return;
  }

  if ((sock = h2o_evloop_socket_accept(listener)) == NULL) {
    return;
  }
  sock->data = listener->data;

  h2o_socket_read_start(sock, on_ws_relay_message);
}

void req_dispose(void *ptr) {
  // If HTTP request is disposed (can happen if the connection goes away, too)
  // ensure we signal that by nulling `req` safely.
  handler_message_t **message_ptr = (handler_message_t **)ptr;
  handler_message_t *handler_message = *message_ptr;

  switch (handler_message->type) {
  case handler_message_http: {
    request_message_t *message = &handler_message->payload.http.msg;
    pthread_mutex_lock(&message->mutex);

    // NULL generator signals completion:
    // https://github.com/h2o/h2o/blob/c54c63285b52421da2782f028022647fc2ea3dd1/lib/core/request.c#L529-L530
    bool dispose_req_message = message->req && message->req->_generator == NULL;

    message->req = NULL;

    if (dispose_req_message) {
      pthread_mutex_unlock(&message->mutex);
      pthread_mutex_destroy(&message->mutex);
      free(handler_message);
    } else {
      pthread_mutex_unlock(&message->mutex);
    }
    break;
  }
  default:
    break;
  }
}

int event_loop_req_handler(h2o_handler_t *self, h2o_req_t *req) {
  if (requests_in_flight > 0 && req->version >= 0x0200) {
    // If HTTP/2 or above is used, they can return results of requests
    // before the previous result is retrieved, so we'll try to proxy it
    // to another worker.
    h2o_req_overrides_t *overrides = h2o_mem_alloc_pool(&req->pool, h2o_req_overrides_t, 1);

    *overrides = (h2o_req_overrides_t){NULL};
    overrides->use_proxy_protocol = false;
    overrides->proxy_preserve_host = true;
    overrides->forward_close_connection = true;

    prepare_req_for_reprocess(req);

    // request reprocess (note: path may become an empty string, to which one of the target URL
    // within the socketpool will be right-padded when lib/core/proxy connects to upstream; see
    // #1563)
    h2o_iovec_t path = h2o_build_destination(req, NULL, 0, 0);
    h2o_reprocess_request(req, req->method, req->scheme, req->authority, path, overrides, 0);
    return 0;
  }
  requests_in_flight++;

  handler_message_t *msg = malloc(sizeof(*msg));
  msg->super = (h2o_multithread_message_t){{NULL}};
  msg->type = handler_message_http;
  msg->payload.http.msg = (request_message_t){.req = req, .process = MyPid};
  h2o_is_websocket_handshake(req, &msg->payload.http.msg.ws_client_key);
  msg->payload.http.websocket_upgrade = msg->payload.http.msg.ws_client_key != NULL;
  if (msg->payload.http.websocket_upgrade) {
    ptls_openssl_random_bytes((void *)msg->payload.http.msg.ws_uuid, 16);
    // Variant:
    // > Set the two most significant bits (bits 6 and 7) of the
    // > clock_seq_hi_and_reserved to zero and one, respectively.
    msg->payload.http.msg.ws_uuid[8] = (msg->payload.http.msg.ws_uuid[8] & 0x3f) | 0x80;
    // Version:
    // > Set the four most significant bits (bits 12 through 15) of the
    // > time_hi_and_version field to the 4-bit version number from
    // > Section 4.1.3.
    msg->payload.http.msg.ws_uuid[6] = (msg->payload.http.msg.ws_uuid[6] & 0x0f) | (4 << 4);
  }

  // Track request deallocation
  handler_message_t **msgref =
      (handler_message_t **)h2o_mem_alloc_shared(&req->pool, sizeof(msg), req_dispose);
  *msgref = msg;

  h2o_multithread_send_message(&handler_receiver, &msg->super);
  return 0;
}
