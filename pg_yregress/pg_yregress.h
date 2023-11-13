#ifndef PG_YREGRESS_H
#define PG_YREGRESS_H

#include <unistd.h>

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
      char *host;
      char *username;
      char *dbname;
      int port;
      char *password;
    } unmanaged;
  } info;
  PGconn *conn;
  pid_t pid;
  struct fy_node *node;
  bool is_default;
  bool restarted;
  bool used;
  struct {
    Oid json;
    Oid jsonb;
    Oid boolean;
  } types;
} yinstance;

typedef enum {
  default_instance_found = 0,
  default_instance_not_found = 1,
  default_instance_ambiguous = 2
} default_yinstance_result;

void yinstance_start(yinstance *instance);

typedef enum {
  yinstance_connect_success,
  yinstance_connect_failure,
  yinstance_connect_error
} yinstance_connect_result;

yinstance_connect_result yinstance_connect(yinstance *instance);

default_yinstance_result default_instance(struct fy_node *instances, yinstance **instance);

iovec_t yinstance_name(yinstance *instance);

void restart_instance(yinstance *instance);

/**
 * List of all instances
 */
extern struct fy_node *instances;

/**
 * Cleans up instances (terminates, removes directories, etc.)
 */
void instances_cleanup();

typedef enum {
  ytest_kind_none,
  ytest_kind_query,
  ytest_kind_steps,
  ytest_kind_tests,
  ytest_kind_restart
} ytest_kind;

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
  bool commit;
  bool negative;
  bool reset;
} ytest;

bool ytest_run(ytest *test);
bool ytest_run_without_transaction(ytest *test);

iovec_t ytest_name(ytest *test);

extern char *pg_config;
extern char bindir[FILENAME_MAX];
extern char sharedir[FILENAME_MAX];
extern char pkglibdir[FILENAME_MAX];

void register_sighandler();

// Utilities

void trim_trailing_whitespace(char *str);

// YAML
bool fy_node_is_boolean(struct fy_node *node);

bool fy_node_get_boolean(struct fy_node *node);

// TAP
extern FILE *tap_file;
extern int tap_counter;

#endif // PG_YREGRESS_H
