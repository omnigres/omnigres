#include <getopt.h>
#include <pwd.h>
#include <stdio.h>

#include "pg_yregress.h"

FILE *tap_file;
int tap_counter = 0;

void meta_free(struct fy_node *fyn, void *meta, void *user) { free(user); }

static bool populate_ytest_from_fy_node(struct fy_document *fyd, struct fy_node *test,
                                        struct fy_node *instances, bool managed) {
  ytest *y_test = calloc(sizeof(*y_test), 1);
  y_test->node = test;
  y_test->commit = false;
  y_test->negative = false;
  switch (fy_node_get_type(test)) {
  case FYNT_SCALAR:
    y_test->name.base = fy_node_get_scalar(test, &y_test->name.len);
    y_test->kind = ytest_kind_query;
    y_test->info.query.query.base = fy_node_get_scalar(test, &y_test->info.query.query.len);

    // Find default instance
    switch (default_instance(instances, &y_test->instance)) {
    case default_instance_not_found:
      fprintf(stderr, "Test %.*s has no default instance to choose from",
              (int)IOVEC_STRLIT(y_test->name));
      return false;
    case default_instance_found:
      y_test->instance->used = true;
      break;
    case default_instance_ambiguous:
      fprintf(stderr, "There's no default instance for test `%.*s` to use",
              (int)IOVEC_STRLIT(y_test->name));
      return false;
    }
    break;
  case FYNT_MAPPING:
    y_test->name.base =
        fy_node_mapping_lookup_scalar_by_simple_key(test, &y_test->name.len, STRLIT("name"));

    {
      // Should we commit after this test?
      struct fy_node *commit = fy_node_mapping_lookup_by_string(test, STRLIT("commit"));

      if (commit != NULL) {
        if (!fy_node_is_boolean(commit)) {
          fprintf(stderr, "commit should be a boolean, got: %s",
                  fy_emit_node_to_string(commit, FYECF_DEFAULT));
          return false;
        }
        y_test->commit = fy_node_get_boolean(commit);
      }
    }

    {
      // Is the test meant to be negative? As in "if test succeeds, it's a failure"
      struct fy_node *reset = fy_node_mapping_lookup_by_string(test, STRLIT("reset"));
      y_test->reset = false;

      if (reset != NULL) {
        if (!fy_node_is_boolean(reset)) {
          fprintf(stderr, "reset should be a boolean, got: %s",
                  fy_emit_node_to_string(reset, FYECF_DEFAULT));
          return false;
        }
        y_test->reset = fy_node_get_boolean(reset);
      }
    }

    {
      // Is the test meant to be negative? As in "if test succeeds, it's a failure"
      struct fy_node *negative = fy_node_mapping_lookup_by_string(test, STRLIT("negative"));

      if (negative != NULL) {
        if (!fy_node_is_boolean(negative)) {
          fprintf(stderr, "negative should be a boolean, got: %s",
                  fy_emit_node_to_string(negative, FYECF_DEFAULT));
          return false;
        }
        y_test->negative = fy_node_get_boolean(negative);
      }
    }

    // Determine the instance to run

    {
      // Is it explicitly specified?
      struct fy_node *instance_ref = fy_node_mapping_lookup_by_string(test, STRLIT("instance"));
      if (instance_ref != NULL) {
        // Must be a scalar
        if (!fy_node_is_scalar(instance_ref)) {
          fprintf(stderr, "Test %.*s instance reference is not a scalar: %s",
                  (int)IOVEC_STRLIT(y_test->name),
                  fy_emit_node_to_string(instance_ref, FYECF_DEFAULT));
          return false;
        }

        // Try to find the instance
        struct fy_node *instance = fy_node_mapping_lookup_value_by_key(instances, instance_ref);
        // If it is not found, bail
        if (instance == NULL) {
          fprintf(stderr, "Test %.*s refers to an unknown instance %.*s",
                  (int)IOVEC_STRLIT(y_test->name), (int)IOVEC_STRLIT(y_test->name));
          return false;
        } else {
          y_test->instance = (yinstance *)fy_node_get_meta(instance);
          y_test->instance->used = true;
        }

      } else {
        // Not explicitly specified
        switch (default_instance(instances, &y_test->instance)) {
        case default_instance_not_found:
          fprintf(stderr, "Test %.*s has no default instance to choose from",
                  (int)IOVEC_STRLIT(y_test->name));
          return false;
        case default_instance_found:
          y_test->instance->used = true;
          break;
        case default_instance_ambiguous:
          fprintf(stderr, "Test %.*s has instance specified to choose from multiple instances",
                  (int)IOVEC_STRLIT(y_test->name));
          return false;
        }
      }
    }

    y_test->kind = ytest_kind_none;
    // Instruction found
    bool instruction_found = false;

    {
      // Are we running a single query?
      struct fy_node *query = fy_node_mapping_lookup_by_string(test, STRLIT("query"));
      if (query != NULL) {
        if (!fy_node_is_scalar(query)) {
          fprintf(stderr, "query must be a scalar, got: %s",
                  fy_emit_node_to_string(query, FYECF_DEFAULT));
          return false;
        }
        y_test->kind = ytest_kind_query;
        y_test->info.query.query.base = fy_node_get_scalar(query, &y_test->info.query.query.len);
        instruction_found = true;
      }
    }

    {
      // Are we running steps?
      struct fy_node *steps = fy_node_mapping_lookup_by_string(test, STRLIT("steps"));
      if (steps != NULL) {

        // Can't have two instructions
        if (instruction_found) {
          fprintf(stderr, "Test %.*s has conflicting type", (int)IOVEC_STRLIT(y_test->name));
          return false;
        }

        if (!fy_node_is_sequence(steps)) {
          fprintf(stderr, "test steps must be a sequence, got: %s",
                  fy_emit_node_to_string(test, FYECF_DEFAULT));
          return false;
        }

        void *iter = NULL;
        struct fy_node *step;

        while ((step = fy_node_sequence_iterate(steps, &iter)) != NULL) {
          if (!populate_ytest_from_fy_node(fyd, step, instances, managed)) {
            return false;
          }
        }

        y_test->kind = ytest_kind_steps;
        instruction_found = true;
      }
    }

    {
      // Are we running subtests?
      struct fy_node *tests = fy_node_mapping_lookup_by_string(test, STRLIT("tests"));
      if (tests != NULL) {

        // Can't have two instructions
        if (instruction_found) {
          fprintf(stderr, "Test %.*s has conflicting type", (int)IOVEC_STRLIT(y_test->name));
          return false;
        }

        if (!fy_node_is_sequence(tests)) {
          fprintf(stderr, "test steps must be a sequence, got: %s",
                  fy_emit_node_to_string(test, FYECF_DEFAULT));
          return false;
        }

        void *iter = NULL;
        struct fy_node *step;

        while ((step = fy_node_sequence_iterate(tests, &iter)) != NULL) {
          if (!populate_ytest_from_fy_node(fyd, step, instances, managed)) {
            return false;
          }
        }

        y_test->kind = ytest_kind_tests;
        instruction_found = true;
      }
    }

    {
      // Are we restarting?
      struct fy_node *restart = fy_node_mapping_lookup_by_string(test, STRLIT("restart"));
      if (restart != NULL) {
        if (!managed) {
          fprintf(stderr, "Can't use `restart` steps in unmanaged instances\n");
          return false;
        }
        y_test->kind = ytest_kind_restart;
        instruction_found = true;
      }
    }

    {
      // Is this test being skipped?
      struct fy_node *skip = fy_node_mapping_lookup_by_string(test, STRLIT("skip"));
      if (skip != NULL) {
        if (!(fy_node_is_boolean(skip) && !fy_node_get_boolean(skip))) {
          instruction_found = true;
        }
      }
    }

    {
      // Is this test being developed?
      struct fy_node *todo = fy_node_mapping_lookup_by_string(test, STRLIT("todo"));
      if (todo != NULL) {
        if (!(fy_node_is_boolean(todo) && !fy_node_get_boolean(todo))) {
          instruction_found = true;
        }
      }
    }

    if (!instruction_found) {
      fprintf(
          stderr,
          "Test %.*s doesn't have a valid instruction (any of: query, steps, tests, skip, todo)",
          (int)IOVEC_STRLIT(ytest_name(y_test)));
      return false;
    }

    break;
  default:
    fprintf(stderr, "test member has an unsupported shape: %s",
            fy_emit_node_to_string(test, FYECF_DEFAULT));
  }
  fy_node_set_meta(test, y_test);
  fy_document_register_meta(fyd, meta_free, NULL);
  return true;
}

struct fy_node *instances;

static int execute_document(struct fy_document *fyd, bool managed, char *host, int port,
                            char *username, char *dbname, char *password) {
  struct fy_node *root = fy_document_root(fyd);
  struct fy_node *original_root = fy_node_copy(fyd, root);
  if (!fy_node_is_mapping(root)) {
    fprintf(stderr, "document root must be a mapping");
    return 1;
  }

  // Get Postgres instances, if any
  instances = fy_node_mapping_lookup_by_string(root, STRLIT("instances"));
  // Get a Postgres instance, if any
  struct fy_node *instance = fy_node_mapping_lookup_by_string(root, STRLIT("instance"));

  if (!managed && (instance != NULL || instances != NULL)) {
    fprintf(
        stderr,
        "Can't have `instance` or `instances` keys when testing against an unmanaged instance\n");
    return 1;
  }

  // Register instance cleanup
  atexit(instances_cleanup);

  // If none specified, create a default one
  if (instances == NULL) {
    instances = fy_node_create_mapping(fyd);

    struct fy_node *default_instance_node = fy_node_create_scalar(fyd, STRLIT("default"));
    fy_node_mapping_append(instances, default_instance_node,
                           instance == NULL ? fy_node_create_mapping(fyd)
                                            : fy_node_copy(fyd, instance));
  } else if (instance != NULL) {
    fprintf(stderr, "can't specify instances and instance at the same time, pick one");
    return 1;
  }

  if (!fy_node_is_mapping(instances)) {
    fprintf(stderr, "instances must be a mapping, got: %s",
            fy_emit_node_to_string(instances, FYECF_DEFAULT));
    return 1;
  }

  // Prepare instance metadata
  {
    bool have_default = false;
    void *iter = NULL;
    struct fy_node_pair *instance_pair;
    while ((instance_pair = fy_node_mapping_iterate(instances, &iter)) != NULL) {
      struct fy_node *name = fy_node_pair_key(instance_pair);
      struct fy_node *instance = fy_node_pair_value(instance_pair);

      yinstance *y_instance = calloc(sizeof(*y_instance), 1);
      y_instance->managed = managed;
      if (!managed) {
        y_instance->info.unmanaged.host = host;
        y_instance->info.unmanaged.username = username;
        y_instance->info.unmanaged.dbname = dbname;
        y_instance->info.unmanaged.port = port;
        y_instance->info.unmanaged.password = password;
      }
      y_instance->ready = false;
      y_instance->node = instance;

      // Before we process tests, we consider the instance to be unused
      y_instance->used = false;

      y_instance->name.base = fy_node_get_scalar(name, &y_instance->name.len);

      fy_node_set_meta(instance, y_instance);
      fy_document_register_meta(fyd, meta_free, NULL);

      switch (fy_node_get_type(instance)) {
      case FYNT_SCALAR:
        break;
      case FYNT_MAPPING:
        y_instance->name.base = fy_node_get_scalar(name, &y_instance->name.len);

        struct fy_node *init = fy_node_mapping_lookup_by_string(y_instance->node, STRLIT("init"));
        if (init != NULL) {
          if (!fy_node_is_sequence(init)) {
            fprintf(stderr, "instance.init has an unsupported shape: %s",
                    fy_emit_node_to_string(instance, FYECF_DEFAULT));

            return 1;
          }

          // Init steps are the same as tests
          {

            // To ensure that init steps are always on the right instance, we only allow
            // _this_ instance to be used
            struct fy_node *self_instances = fy_node_create_mapping(fyd);
            struct fy_node *instance_copy = fy_node_copy(fyd, instance);
            fy_node_set_meta(instance_copy, y_instance);
            fy_node_mapping_append(self_instances, fy_node_create_scalar(fyd, STRLIT("default")),
                                   instance_copy);

            void *init_iter = NULL;
            struct fy_node *init_step;
            while ((init_step = fy_node_sequence_iterate(init, &init_iter)) != NULL) {
              if (!populate_ytest_from_fy_node(fyd, init_step, self_instances, managed)) {
                return 1;
              }
              // Ensure it points to the correct instance
              // (otherwise it will either get a default or will try to use
              //  a specified instance, which doesn't make sense in this context)
              ytest *y_init_step = (ytest *)fy_node_get_meta(init_step);
              y_init_step->instance = y_instance;
            }
          }
        }

        struct fy_node *is_default =
            fy_node_mapping_lookup_by_string(y_instance->node, STRLIT("default"));

        if (is_default != NULL) {
          // Ensure `default` is a boolean
          if (!fy_node_is_boolean(is_default)) {
            fprintf(stderr, "instance.default must be a boolean, got: %s",
                    fy_emit_node_to_string(instance, FYECF_DEFAULT));
            return 1;
          }
          // Ensure no default has been picked already
          if (have_default) {
            fprintf(stderr, "instance.default must be set only for one instance");
            return 1;
          }
          have_default = true;
          y_instance->is_default = fy_node_get_boolean(is_default);
        }

        break;
      default:
        fprintf(stderr, "instance member has an unsupported shape: %s",
                fy_emit_node_to_string(instance, FYECF_DEFAULT));
        return 1;
      }
    }
  }

  // Get tests
  struct fy_node *tests = fy_node_mapping_lookup_by_string(root, STRLIT("tests"));

  if (tests == NULL) {
    printf("No tests specified");
    return 1;
  }

  if (!fy_node_is_sequence(tests)) {
    fprintf(stderr, "tests must be a sequence, got: %s",
            fy_emit_node_to_string(tests, FYECF_DEFAULT));
    return 1;
  }

  // Prepare test metadata
  {
    void *iter = NULL;
    struct fy_node *test;
    while ((test = fy_node_sequence_iterate(tests, &iter)) != NULL) {
      if (!populate_ytest_from_fy_node(fyd, test, instances, managed)) {
        return 1;
      }
    }
  }

  // Count used instances
  int used_instances = 0;
  {
    void *iter = NULL;
    struct fy_node_pair *instance_pair;
    while ((instance_pair = fy_node_mapping_iterate(instances, &iter)) != NULL) {
      yinstance *y_instance = (yinstance *)fy_node_get_meta(fy_node_pair_value(instance_pair));
      if (y_instance->used) {
        used_instances++;
      }
    }
  }

  // Prepare TAP output
  int test_count = 1 + fy_node_sequence_item_count(tests) + used_instances;
  fprintf(tap_file, "TAP version 14\n");
  fprintf(tap_file, "1..%d\n", test_count);
  {
    size_t name_len;
    const char *name = fy_node_mapping_lookup_scalar_by_simple_key(root, &name_len, STRLIT("name"));
    if (name != NULL) {
      fprintf(tap_file, "# Test suite: %.*s\n", (int)name_len, name);
    }
  }

  // Initialize used instances
  {
    fprintf(tap_file, "# Initializing instances\n");
    void *iter = NULL;
    struct fy_node_pair *instance_pair;
    while ((instance_pair = fy_node_mapping_iterate(instances, &iter)) != NULL) {
      yinstance *y_instance = (yinstance *)fy_node_get_meta(fy_node_pair_value(instance_pair));
      if (y_instance->used) {
        yinstance_start(y_instance);
        // If instance is still not ready, it means there was a recoverable error
        if (!y_instance->ready) {
          // We can't do much about it, bail.
          fprintf(tap_file, "Bail out! Can't connect to instance `%.*s`",
                  (int)IOVEC_STRLIT(y_instance->name));
          return 1;
        }
        yinstance_connect(y_instance);
      }
    }
    fprintf(tap_file, "# Done initializing instances\n");
  }

  // Run tests
  bool succeeded = true;
  {
    void *iter = NULL;
    struct fy_node *test;

    while ((test = fy_node_sequence_iterate(tests, &iter)) != NULL) {
      ytest *y_test = fy_node_get_meta(test);
      if (!ytest_run(y_test)) {
        succeeded = false;
      }
      // Currently, ytest_run() may change the node by replacing node of one type
      // with another, and we need to continue from there on.
      // This is a little hacky and exploits the internal knowledge of how the
      // iterator works.
      // See https://github.com/pantoniou/libfyaml/issues/84, if something will
      // come out of this, it'll make this better
      iter = y_test->node;
    }
  }

  // Remove `env`
  {
    struct fy_node *env = fy_node_mapping_lookup_key_by_string(root, STRLIT("env"));
    if (env != NULL) {
      fy_node_mapping_remove_by_key(root, env);
    }
  }
  {
    struct fy_node *env = fy_node_mapping_lookup_key_by_string(original_root, STRLIT("env"));
    if (env != NULL) {
      fy_node_mapping_remove_by_key(original_root, env);
    }
  }

  return succeeded ? 0 : 1;
}

char *pg_config;
char bindir[FILENAME_MAX];
char sharedir[FILENAME_MAX];
char pkglibdir[FILENAME_MAX];

static void get_path_from_popen(char *cmd, char *path) {
  FILE *f_cmd = popen(cmd, "r");
  {
    int len = 0;
    if (f_cmd != NULL) {
      char *buf = path;
      while ((buf = fgets(buf + len, sizeof(bindir), f_cmd)) != NULL) {
        len += buf - path;
      }
      // Ensure there's no trailing newline
      path[strcspn(path, "\n")] = 0;

      pclose(f_cmd);
    } else {
      fprintf(stderr, "Can't call %s", cmd);
      exit(1);
    }
  }
}

extern char **environ;

int main(int argc, char **argv) {

  static struct option options[] = {{"host", required_argument, 0, 'h'},
                                    {"username", required_argument, 0, 'U'},
                                    {"dbname", required_argument, 0, 'd'},
                                    {"port", required_argument, 0, 'p'},
                                    {"password", no_argument, 0, 'W'},
                                    {"no-password", no_argument, 0, 'w'},
                                    {0, 0, 0, 0}};

  bool managed = true;
  char *host = "127.0.0.1";
  char *username = getpwuid(getuid())->pw_name;
  char *dbname = username;
  int port = 5432;
  char *password = getenv("PGPASSWORD");

  {
    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "h:U:d:p:Ww", options, &option_index)) != -1) {
      switch (opt) {
      case 'h':
        host = optarg;
        managed = false;
        break;
      case 'U': {
        bool is_original_dbname = dbname == username;
        username = optarg;
        if (is_original_dbname) {
          // Update dbname to match username if it hasn't been changed yet
          dbname = username;
        }
        managed = false;
        break;
      }
      case 'd':
        dbname = optarg;
        managed = false;
        break;
      case 'p':
        port = atoi(optarg);
        managed = false;
        break;
      case 'W':
        password = getpass("Password: ");
        managed = false;
        break;
      case 'w':
        managed = false;
        break;
      default:
        fprintf(stderr,
                "Usage: %s [--host|-h] [--username|-U] [--dbname|-d] [--port|-p] "
                "[-w|--no-password] [-W|--password] file\n",
                argv[0]);
        return 1;
      }
    }
  }

  if (optind < argc) {
    // Before starting, we try to open special FD for tap file
    tap_file = stdout;

    // Handle signals for cleanup, etc.
    register_sighandler();

    // Retrieve hard-coded Postgres configuration
    pg_config = getenv("PGCONFIG") ?: "pg_config";

    // Code below redirects pg_config's stderr to /dev/null
    // to avoid reading it. It can be present when pg_config is compiled
    // with a sanitizer, for example.

    char *bindir_cmd;
    asprintf(&bindir_cmd, "%s --bindir 2>/dev/null", pg_config);
    get_path_from_popen(bindir_cmd, bindir);

    char *sharedir_cmd;
    asprintf(&sharedir_cmd, "%s --sharedir 2>/dev/null", pg_config);
    get_path_from_popen(sharedir_cmd, sharedir);

    char *pkglibdir_cmd;
    asprintf(&pkglibdir_cmd, "%s --pkglibdir 2>/dev/null", pg_config);
    get_path_from_popen(pkglibdir_cmd, pkglibdir);

    struct fy_diag_cfg diag_cfg;
    fy_diag_cfg_default(&diag_cfg);
    diag_cfg.colorize = true;
    diag_cfg.show_position = true;

    // Read the document without resolving aliases (yet)
    struct fy_parse_cfg parse_cfg = {
        .flags = FYPCF_PARSE_COMMENTS | FYPCF_YPATH_ALIASES | FYPCF_COLLECT_DIAG,
        .diag = fy_diag_create(&diag_cfg),
    };

    struct fy_document *fyd = fy_document_build_from_file(&parse_cfg, argv[optind]);

    if (fyd == NULL) {
      exit(1);
    }

    // Prepare `env` dictionary
    struct fy_node *root = fy_document_root(fyd);
    struct fy_node *env = fy_node_create_mapping(fyd);

    for (char **var = environ; *var != NULL; var++) {
      struct fy_node *value = fy_node_create_scalar(fyd, STRLIT(strchr(*var, '=') + 1));
      int varname_len = (int)(strchr(*var, '=') - *var);
      fy_node_set_anchorf(value, "ENV(%.*s)", varname_len, *var);
      fy_node_mapping_append(env, fy_node_create_scalar(fyd, *var, varname_len), value);
    }

    // Add it to the root
    fy_node_mapping_prepend(root, fy_node_create_scalar(fyd, STRLIT("env")), env);

    // Resolve aliases
    fy_document_resolve(fyd);

    if (fyd != NULL) {
      int result = execute_document(fyd, managed, host, port, username, dbname, password);
      return result;
    }
  } else {
    fprintf(stderr, "Usage: py_yregress <test.yml>");
  }
  return 0;
}