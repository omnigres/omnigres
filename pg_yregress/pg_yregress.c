#include <stdio.h>

#include "pg_yregress.h"

void meta_free(struct fy_node *fyn, void *meta, void *user) { free(user); }

static bool populate_ytest_from_fy_node(struct fy_document *fyd, struct fy_node *test,
                                        struct fy_node *instances) {
  ytest *y_test = calloc(sizeof(*y_test), 1);
  y_test->node = test;
  switch (fy_node_get_type(test)) {
  case FYNT_MAPPING:
    y_test->name.base =
        fy_node_mapping_lookup_scalar_by_simple_key(test, &y_test->name.len, STRLIT("name"));

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

      switch (fy_node_get_type(instance)) {
      case FYNT_SCALAR:
        y_instance->name.base = fy_node_get_scalar(name, &y_instance->name.len);
        break;
      case FYNT_MAPPING:
        y_instance->name.base = fy_node_mapping_lookup_scalar_by_simple_key(
            instance, &y_instance->name.len, STRLIT("name"));

        struct fy_node *init = fy_node_mapping_lookup_by_string(y_instance->node, STRLIT("init"));
        if (init != NULL) {
          if (!fy_node_is_sequence(init)) {
            fprintf(stderr, "instance.init has an unsupported shape: %s",
                    fy_emit_node_to_string(instance, FYECF_DEFAULT));

            return 1;
          }
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
  {
    void *iter = NULL;
    struct fy_node *test;

    while ((test = fy_node_sequence_iterate(tests, &iter)) != NULL) {
      ytest *y_test = fy_node_get_meta(test);
      ytest_run(y_test);
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

int main(int argc, char **argv) {

  if (argc >= 2) {
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
    struct fy_parse_cfg parse_cfg = {
        .flags = FYPCF_PARSE_COMMENTS | FYPCF_RESOLVE_DOCUMENT | FYPCF_COLLECT_DIAG,
        .diag = fy_diag_create(&diag_cfg),
    };
    struct fy_document *fyd = fy_document_build_from_file(&parse_cfg, argv[1]);

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