#ifndef PG_YREGRESS_H
#define PG_YREGRESS_H

#include <libfyaml.h>
#include <libpq-fe.h>

typedef struct {
  const char *base;
  size_t len;
} iovec_t;

#define STRLIT(s) s, strlen(s)
#define IOVEC_STRLIT(vec) vec.len, vec.base
#define IOVEC_RSTRLIT(vec) vec.base, vec.len

/**
 * Metainformation for a database instance
 */
typedef struct {
  iovec_t name;
  bool managed;
  bool ready;
  union {
    struct {
      iovec_t datadir;
      uint16_t port;
    } managed;
    struct {
      // TODO
    } unmanaged;
  } info;
  PGconn *conn;
  struct fy_node *node;
} yinstance;

typedef enum {
  default_instance_found = 0,
  default_instance_not_found = 1,
  default_instance_ambiguous = 2
} default_yinstance_result;

void yinstance_start(yinstance *instance);

PGconn *yinstance_connect(yinstance *instance);

default_yinstance_result default_instance(struct fy_node *instances, yinstance **instance);

iovec_t yinstance_name(yinstance *instance);

/**
 * List of all instances
 */
extern struct fy_node *instances;

/**
 * Cleans up instances (terminates, removes directories, etc.)
 */
void instances_cleanup();

typedef enum { ytest_kind_none, ytest_kind_query } ytest_kind;

typedef struct {
  iovec_t name;

  yinstance *instance;
  ytest_kind kind;
  union {
    struct {
      iovec_t query;
    } query;
  } info;
  struct fy_node *node;
  struct fy_document *doc;
} ytest;

void ytest_run(ytest *test);

iovec_t ytest_name(ytest *test);

extern char *pg_config;
extern char bindir[FILENAME_MAX];
extern char sharedir[FILENAME_MAX];
extern char pkglibdir[FILENAME_MAX];

void register_sighandler();

// Utilities

void trim_trailing_whitespace(char *str);

#endif // PG_YREGRESS_H
