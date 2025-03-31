#ifndef OMNI_HTTPD_EVENT_LOOP_H
#define OMNI_HTTPD_EVENT_LOOP_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <wslay/wslay.h>

/**
 * This event loop handles incoming connections
 */
extern h2o_evloop_t *worker_event_loop;

/**
 * Indicates whether the worker should be running.
 */
extern atomic_bool worker_running;

/**
 * Indicates whether the worker should be reloaded
 */
extern atomic_bool worker_reload;

/**
 * Indicates if `worker_event_loop` should be suspended
 */
extern bool event_loop_suspended;

/**
 * Indicates if `worker_event_loop` has been resumed
 */
extern bool event_loop_resumed;

/**
 * Mutex to access condvars like `event_loop_resume_cond` and
 * `event_loop_resume_cond_ack`
 */
extern pthread_mutex_t event_loop_mutex;

/**
 * Signals that `worker_event_loop` should be resumed
 */
extern pthread_cond_t event_loop_resume_cond;

/**
 * Signals that `worker_event_loop` has been resumed
 */
extern pthread_cond_t event_loop_resume_cond_ack;

/**
 * `worker_event_loop` receives HTTP responses through this receiver
 */
extern h2o_multithread_receiver_t event_loop_receiver;

/**
 * `worker_event_loop` receives HTTP responses in this queue
 */
extern h2o_multithread_queue_t *event_loop_queue;

/**
 * UUID for the purpsoe of WebSocket identification
 */
typedef unsigned char websocket_uuid_t[16];

/**
 * `worker_event_loop` envelopes incoming HTTP requests into this data type
 */
typedef struct {
  //  h2o_multithread_message_t super;
  h2o_req_t *req;
  pthread_mutex_t mutex;
  const char *ws_client_key;
  struct wslay_event_on_msg_recv_arg websocket_message;
  websocket_uuid_t ws_uuid;
  pid_t process;
} request_message_t;

typedef struct {
  h2o_multithread_message_t super;
  enum {
    handler_message_http,
    handler_message_websocket_open,
    handler_message_websocket_close,
    handler_message_websocket_message
  } type;
  union {
    struct {
      request_message_t msg;
      bool websocket_upgrade;
    } http;
    struct {
      websocket_uuid_t uuid;
    } websocket_open;
    struct {
      websocket_uuid_t uuid;
    } websocket_close;
    struct {
      websocket_uuid_t uuid;
      enum {
        websocket_text_message,
        websocket_binary_message,
      } opcode;
      uint8_t *message;
      size_t message_len;
    } websocket_message;
  } payload;
} handler_message_t;

/**
 * Main thread's event loop receives `handler_message_t` messages using this receiver
 */
extern h2o_multithread_receiver_t handler_receiver;

/**
 * Main thread's event loop receives `request_message_t` messages in this queue
 */
extern h2o_multithread_queue_t *handler_queue;
void *event_loop(void *arg);

/**
 * Returns socket's accept context
 * @param listener
 * @return
 */
h2o_accept_ctx_t *listener_accept_ctx(h2o_socket_t *listener);

/**
 * @brief Accept socket
 *
 * @param listener
 * @param err
 */
void on_accept(h2o_socket_t *listener, const char *err);

void on_accept_ws_unix(h2o_socket_t *listener, const char *err);

/**
 * `worker_event_loop` should be setup to use this handler
 * @param self
 * @param req
 * @return
 */
int event_loop_req_handler(h2o_handler_t *self, h2o_req_t *req);

/**
 * Send inline HTTP response to `worker_event_loop`
 * @param msg request message
 * @param body body; must be allocated in the pool
 * @param len length
 */
void h2o_queue_send_inline(request_message_t *msg, const char *body, size_t len);

/**
 * Request aborting the connection
 * @param msg request message
 */
void h2o_queue_abort(request_message_t *msg);

/**
 * Request proxying
 * @param msg request message
 * @param url target URL
 * @param preserve_host whether Host header should be preserved
 */
void h2o_queue_proxy(request_message_t *msg, char *url, bool preserve_host);

/**
 * Request upgrade to WebSocket
 * @param msg request message
 */
void h2o_queue_upgrade_to_websocket(request_message_t *msg);

/**
 * Registers event loop receiver. Must be done prior to starting
 * event loop's thread.
 *
 * An internal implementation detail.
 *
 * TODO: redesign event_loop <-> http_worker interaction to simplify this
 */
void event_loop_register_receiver();

/**
 * Sets addr->sun_path to a temporary socket path for the given uuid.
 *
 * This may subtly modify the given uuid.
 *
 * Returns 0 on success or -EINVAL if the socket path is too long.
 */
int websocket_unix_socket_path(struct sockaddr_un *addr, websocket_uuid_t *uuid);

/**
 * unlink(2) the temporary socket path for the given uuid.
 *
 * This may subtly modify the given uuid.
 *
 * Returns 0 on success or -EINVAL if the socket path is too long or -1 and
 * sets errno if unlink fails.
 */
int unlink_websocket_unix_socket(websocket_uuid_t *uuid);

#endif // OMNI_HTTPD_EVENT_LOOP_H
