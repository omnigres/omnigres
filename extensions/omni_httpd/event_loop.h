#ifndef OMNI_HTTPD_EVENT_LOOP_H
#define OMNI_HTTPD_EVENT_LOOP_H

#include <pthread.h>
#include <stdbool.h>

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
 * `worker_event_loop` envelopes incoming HTTP requests into this data type
 */
typedef struct {
  h2o_multithread_message_t super;
  h2o_req_t *req;
  pthread_mutex_t mutex;
} request_message_t;

/**
 * Main thread's event loop receives `request_message_t` messages using this receiver
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

#endif // OMNI_HTTPD_EVENT_LOOP_H
