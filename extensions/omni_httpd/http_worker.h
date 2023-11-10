#ifndef OMNI_HTTP_WORKER_H
#define OMNI_HTTP_WORKER_H

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <executor/spi.h>

#include <h2o.h>

#include <libgluepg_stc.h>

/**
 * @brief Each listener has a context
 *
 */
typedef struct st_listener_ctx {
  /**
   * @brief Query plan
   *
   */
  SPIPlanPtr plan;
  /**
   * @brief Listener's MemoryContext
   *
   */
  MemoryContext memory_context;
  /**
   * @brief Role ID
   *
   */
  Oid role_id;
  /**
   * @brief Is role a superuser?
   */
  bool role_is_superuser;
  /**
   * @brief Associated socket
   *
   */
  h2o_socket_t *socket;
  /**
   * @brief Underlying file descriptor
   *
   */
  int fd;
  /**
   * @brief Underlying master file descriptor
   */
  int master_fd;
  /**
   * @brief Accept context
   *
   */
  h2o_accept_ctx_t accept_ctx;
  /**
   * @brief H2O context
   *
   * Inclusion of this context into `listener_ctx` allows us to retrieve `listener_ctx`:
   *
   * ```
   * listener_ctx *lctx = H2O_STRUCT_FROM_MEMBER(listener_ctx, context, req->conn->ctx);
   * ```
   *
   */
  h2o_context_t context;
} listener_ctx;

static inline int listener_ctx_cmp(const listener_ctx *l, const listener_ctx *r);

// This defines clist_listener_context
#define i_tag listener_contexts
#define i_val listener_ctx
#define i_eq c_memcmp_eq
#define i_cmp listener_ctx_cmp
#include <stc/clist.h>

static h2o_globalconf_t config;

/**
 * Sets up H2O server
 */
static void setup_server();

/**
 * @brief Create a H2O listener socket
 *
 * @param fd
 * @param listener_ctx
 * @return int
 */
static int create_listener(int fd, listener_ctx *listener_ctx);

// Defines cset_fd
#define i_val int
#define i_tag fd
#include <stc/cset.h>

/**
 * Accepts a batch of file descriptors from a named socket
 * @param socket_name
 * @return
 */
static cvec_fd_fd accept_fds(char *socket_name);

/**
 * @brief Number of fields in http_request type
 *
 */
#define REQUEST_PLAN_PARAMS 5
/**
 * @brief Parameter index in a prepared query
 *
 */
#define REQUEST_PLAN_PARAM(x) ML99_STRINGIFY(ML99_INC(x))

/**
 * @brief Method parameter index
 *
 */
#define REQUEST_PLAN_METHOD 0
/**
 * @brief Path parameter index
 *
 */
#define REQUEST_PLAN_PATH 1
/**
 * @brief Query string parameter index
 *
 */
#define REQUEST_PLAN_QUERY_STRING 2
/**
 * @brief Body parameter index
 *
 */
#define REQUEST_PLAN_BODY 3
/**
 * @brief Headers parameter index
 *
 */
#define REQUEST_PLAN_HEADERS 4

static int handler(request_message_t *msg);

static h2o_evloop_t *handler_event_loop;

#define HTTP_OUTCOME_RESPONSE 0
#define HTTP_OUTCOME_ABORT 1
#define HTTP_OUTCOME_PROXY 2

#endif // OMNIGRES_HTTP_WORKER_H
