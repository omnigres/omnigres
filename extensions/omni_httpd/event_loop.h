#ifndef OMNI_HTTPD_EVENT_LOOP_H
#define OMNI_HTTPD_EVENT_LOOP_H

#include <pthread.h>
#include <stdbool.h>

extern h2o_evloop_t *worker_event_loop;

extern atomic_bool worker_running;
extern atomic_bool worker_reload;
extern bool event_loop_suspended;
extern bool event_loop_resumed;
extern pthread_mutex_t event_loop_mutex;
extern pthread_cond_t event_loop_resume_cond;
extern pthread_cond_t event_loop_resume_cond_ack;
extern pthread_cond_t event_loop_request_cond;
extern h2o_multithread_receiver_t event_loop_receiver;
extern h2o_multithread_queue_t *event_loop_queue;

typedef struct {
  h2o_multithread_message_t super;
  h2o_req_t *req;
  pthread_mutex_t mutex;
} request_message_t;

extern h2o_multithread_receiver_t handler_receiver;
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

int event_loop_req_handler(h2o_handler_t *self, h2o_req_t *req);

void h2o_queue_send_inline(request_message_t *msg, const char *body, size_t len);

#endif // OMNI_HTTPD_EVENT_LOOP_H
