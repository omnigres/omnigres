#include <assert.h>
#include <stdio.h>

#include "pg_yregress.h"

FILE *tap_file;
int tap_counter = 0;

void meta_free(struct fy_node *fyn, void *meta, void *user) { free(user); }

static bool populate_ytest_from_fy_node(struct fy_document *fyd, struct fy_node *test,
                                        struct fy_node *instances) {
  ytest *y_test = calloc(sizeof(*y_test), 1);
  y_test->node = test;
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

    struct fy_node *commit = fy_node_mapping_lookup_by_string(test, STRLIT("commit"));

    if (commit != NULL) {
      if (!fy_node_is_boolean(commit)) {
        fprintf(stderr, "commit should be a boolean, got: %s",
                fy_emit_node_to_string(commit, FYECF_DEFAULT));
        return false;
      }
      y_test->commit = fy_node_get_boolean(commit);
    } else {
      y_test->commit = false;
    }

    // Determine the instance to run

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
      }

    } else {
      // Not explicitly specified
      switch (default_instance(instances, &y_test->instance)) {
      case default_instance_not_found:
        fprintf(stderr, "Test %.*s has no default instance to choose from",
                (int)IOVEC_STRLIT(y_test->name));
        return false;
      case default_instance_found:
        break;

      case default_instance_ambiguous:
        fprintf(stderr, "Test %.*s has instance specified to choose from multiple instances",
                (int)IOVEC_STRLIT(y_test->name));
        return false;
      }
    }

    y_test->kind = ytest_kind_none;
    // Instruction found
    bool instruction_found = false;

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
        if (!populate_ytest_from_fy_node(fyd, step, instances)) {
          return false;
        }
      }

      y_test->kind = ytest_kind_steps;
      instruction_found = true;
    }

    // Are we restarting?
    struct fy_node *restart = fy_node_mapping_lookup_by_string(test, STRLIT("restart"));
    if (restart != NULL) {
      y_test->kind = ytest_kind_restart;
      instruction_found = true;
    }

    if (!instruction_found) {
      fprintf(stderr, "Test %.*s doesn't have a valid instruction (any of: query, step)",
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

static int count_tests(struct fy_node *tests) {
  assert(fy_node_is_sequence(tests));
  void *iter = NULL;
  struct fy_node *test;
  int i = 0;
  while ((test = fy_node_sequence_iterate(tests, &iter)) != NULL) {
    i++;
    ytest *y_test = (ytest *)fy_node_get_meta(test);
    if (y_test->kind == ytest_kind_steps) {
      struct fy_node *steps = fy_node_mapping_lookup_by_string(test, STRLIT("steps"));
      assert(steps != NULL);
      i += count_tests(steps);
    }
  }
  return i;
}

static int execute_document(struct fy_document *fyd, FILE *out) {
  struct fy_node *root = fy_document_root(fyd);
  struct fy_node *original_root = fy_node_copy(fyd, root);
  if (!fy_node_is_mapping(root)) {
    fprintf(stderr, "document root must be a mapping");
    return 1;
  }

  // Get Postgres instances
  instances = fy_node_mapping_lookup_by_string(root, STRLIT("instances"));

  // Register instance cleanup
  atexit(instances_cleanup);

  // If none specified, create a default one
  if (instances == NULL) {
    instances = fy_node_create_mapping(fyd);
    struct fy_node *default_instance_node = fy_node_create_scalar(fyd, STRLIT("default"));
    fy_node_mapping_append(instances, default_instance_node, fy_node_create_mapping(fyd));
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
      // By default, we assume it to be managed (unless overridden)
      y_instance->managed = true;
      y_instance->ready = false;
      y_instance->node = instance;

      y_instance->name.base = fy_node_get_scalar(name, &y_instance->name.len);
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
            void *init_iter = NULL;
            struct fy_node *init_step;
            while ((init_step = fy_node_sequence_iterate(init, &init_iter)) != NULL) {
              if (!populate_ytest_from_fy_node(fyd, init_step, instances)) {
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
      fy_node_set_meta(instance, y_instance);
      fy_document_register_meta(fyd, meta_free, NULL);
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
      if (!populate_ytest_from_fy_node(fyd, test, instances)) {
        return 1;
      }
    }
  }

  // Run tests
  int test_count = 1 + count_tests(tests); // count from 1, so add 1
  fprintf(tap_file, "TAP version 14\n");
  fprintf(tap_file, "1..%d\n", test_count);

  {
    void *iter = NULL;
    struct fy_node *test;

    while ((test = fy_node_sequence_iterate(tests, &iter)) != NULL) {
      ytest *y_test = fy_node_get_meta(test);
      ytest_run(y_test);
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

  // Output the result sheet
  fy_emit_document_to_fp(
      fyd, FYECF_MODE_ORIGINAL | FYECF_OUTPUT_COMMENTS | FYECF_INDENT_2 | FYECF_WIDTH_80, out);

  return fy_node_compare(original_root, root) ? 0 : 1;
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

pid_t pgid;

extern char **environ;

#define FD_TAP 1001

int main(int argc, char **argv) {

  if (argc >= 2) {
    // Before starting, we try to open special FD for tap file
    tap_file = fdopen(FD_TAP, "w");
    // If not available, write to /dev/null
    if (tap_file == NULL) {
      tap_file = fopen("/dev/null", "w");
    }
    // Get a process group
    pgid = getpgrp();

    // Hanle signals for cleanup, etc.
    register_sighandler();

    // Retrieve hard-coded Postgres configuration
    pg_config = getenv("PGCONFIG") ?: "pg_config";

    char *bindir_cmd;
    asprintf(&bindir_cmd, "%s --bindir", pg_config);
    get_path_from_popen(bindir_cmd, bindir);

    char *sharedir_cmd;
    asprintf(&sharedir_cmd, "%s --sharedir", pg_config);
    get_path_from_popen(sharedir_cmd, sharedir);

    char *pkglibdir_cmd;
    asprintf(&pkglibdir_cmd, "%s --pkglibdir", pg_config);
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

    struct fy_document *fyd = fy_document_build_from_file(&parse_cfg, argv[1]);

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
      FILE *out = argc >= 3 ? fopen(argv[2], "w") : stdout;
      int result = execute_document(fyd, out);
      fclose(out);
      return result;
    }
  } else {
    fprintf(stderr, "Usage: py_yregress <test.yml> [output.yaml]");
  }
  return 0;
}