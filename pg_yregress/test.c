#include <assert.h>

#include <libpq-fe.h>

#include "pg_yregress.h"

union fy_scalar_attributes {
  struct {
    bool definitely_not_a_bool : 1;
  };
  // this is done so that this can be used as meta pointer
  void *ptr;
};

_Static_assert(sizeof(union fy_scalar_attributes) <= sizeof(void *),
               "fy_scalar_attributes should fit into a pointer so we don't have to clean it up");

static bool definitely_not_a_bool(struct fy_node *node) {
  if (node == NULL)
    return false;
  if (fy_node_get_meta(node) == NULL)
    return false;
  union fy_scalar_attributes attrs = {.ptr = fy_node_get_meta(node)};
  return attrs.definitely_not_a_bool;
}

int fy_token_cmp(struct fy_token *fyt1, struct fy_token *fyt2);

static int boolean_aware_cmp_fn(struct fy_node *fyn_a, struct fy_node *fyn_b, void *arg) {
  if (fyn_a == fyn_b)
    return 0;
  if (!fyn_a)
    return 1;
  if (!fyn_b)
    return -1;
  if (fy_node_is_boolean(fyn_a) && fy_node_is_boolean(fyn_b)) {
    if (definitely_not_a_bool(fyn_a) || definitely_not_a_bool(fyn_b)) {
      goto token_comparison;
    }
    bool a = fy_node_get_boolean(fyn_a);
    bool b = fy_node_get_boolean(fyn_b);
    if (a == b) {
      return 0;
    }
    if (!a) {
      return -1;
    }
    return 1;
  }
token_comparison:
  return fy_token_cmp(fy_node_get_scalar_token(fyn_a), fy_node_get_scalar_token(fyn_b));
}

static void notice_receiver(struct fy_node *notices, const PGresult *result) {
  char *notice = strdup(PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY));
  struct fy_document *doc = fy_node_document(notices);
  fy_node_sequence_append(notices, fy_node_create_scalar(doc, STRLIT(notice)));
}

bool ytest_run_internal(PGconn *default_conn, ytest *test, bool in_transaction, int sub_test,
                        bool *errored) {
#define taprintf(str, ...) fprintf(tap_file, "%*s" str, sub_test * 4, "", ##__VA_ARGS__)
  // This will be used for TAP output
  struct fy_document *doc = fy_node_document(test->node);
  struct fy_node *original_node = fy_node_copy(doc, test->node);

  struct fy_node *skip = fy_node_mapping_lookup_by_string(test->node, STRLIT("skip"));

  if (skip != NULL) {
    bool allocated = false;
    iovec_t reason = {.base = "", .len = 0};
    if (fy_node_is_boolean(skip)) {
      if (!fy_node_get_boolean(skip)) {
        goto proceed;
      }
    } else if (fy_node_is_scalar(skip)) {
      reason.base = fy_node_get_scalar(skip, &reason.len);
    } else {
      reason.base = fy_emit_node_to_string(skip, FYECF_NO_ENDING_NEWLINE | FYECF_WIDTH_INF |
                                                     FYECF_MODE_FLOW_ONELINE);
      reason.len = strlen(reason.base);
      allocated = true;
    }
    tap_counter++;
    taprintf("ok %d - %.*s # SKIP %.*s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)),
             (int)IOVEC_STRLIT(reason));
    if (allocated) {
      free((void *)reason.base);
    }
    return true;
  }

proceed:

  assert(test->instance != NULL);
  PGconn *conn;
  // create connection to other database each time
  if (test->instance->managed && test->database.base != NULL) {
    char *conninfo;

    asprintf(&conninfo, "host=127.0.0.1 port=%d dbname=%s user=yregress",
             test->instance->info.managed.port, test->database.base);

    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) == CONNECTION_BAD) {
      tap_counter++;
      taprintf("not ok %d - %.*s # Can't connect: %s\n", tap_counter,
               (int)IOVEC_STRLIT(ytest_name(test)), PQerrorMessage(conn));
      return false;
    }
  } else if (default_conn == NULL) {
    struct fy_node *test_container = fy_node_get_parent(test->node);
    assert(fy_node_is_sequence(test_container));
    struct fy_node *maybe_instance = fy_node_get_parent(test_container);
    // It is either a root or an instance
    assert(fy_node_is_mapping(maybe_instance));

    // If connection is not present, connect. Here we're dealing with the
    // possibility that a test before required a restart.
    // (unless this test is part of the initialization sequence)
    switch (yinstance_connect(test->instance)) {
    case yinstance_connect_success:
      break;
    default:
      tap_counter++;
      taprintf("not ok %d - %.*s # Can't reconnect to the instance %.*s\n", tap_counter,
               (int)IOVEC_STRLIT(ytest_name(test)),
               (int)IOVEC_STRLIT(yinstance_name(test->instance)));
      return false;
    }
    conn = test->instance->conn;
  } else {
    conn = default_conn;
  }
  if (test->reset && !in_transaction) {
    PQreset(conn);
  }
  assert(conn != NULL);

  // We will store notices here
  struct fy_node *notices = fy_node_create_sequence(doc);
  PQnoticeReceiver prev_notice_receiver = PQsetNoticeReceiver(conn, NULL, NULL);
  assert(prev_notice_receiver != NULL);
  // Subscribe to notices
  PQsetNoticeReceiver(conn, (PQnoticeReceiver)notice_receiver, (void *)notices);

  switch (test->kind) {
  case ytest_kind_query: {

    if (!in_transaction) {
      PGresult *begin_result = PQexec(conn, "begin");
      PQclear(begin_result);
    }

    // Prepare the query
    char *query;
    asprintf(&query, "%.*s", (int)IOVEC_STRLIT(test->info.query.query));
    // Execute the query

    PGresult *result;

    int result_format = 0;      // result format,  characters by default
    bool binary_params = false; // parameter format, characters by default

    struct fy_node *binary = fy_node_mapping_lookup_value_by_string(test->node, STRLIT("binary"));

    if (binary != NULL) {
      if (fy_node_is_boolean(binary) && fy_node_get_boolean(binary)) {
        result_format = 1;
        binary_params = true;
      }
      if (fy_node_compare_text(binary, STRLIT("results"))) {
        result_format = 1;
      } else if (fy_node_compare_text(binary, STRLIT("params"))) {
        binary_params = true;
      }
    }

    struct fy_node *params = fy_node_mapping_lookup_by_string(test->node, STRLIT("params"));

    if (params == NULL) {
      result = PQexecParams(conn, query, 0, NULL, (const char **)NULL, NULL, NULL, result_format);
    } else if (fy_node_is_sequence(params)) {
      int nparams = fy_node_sequence_item_count(params);
      const char **values = calloc(nparams, sizeof(char *));
      int *param_formats = calloc(nparams, sizeof(int));
      int *param_length = calloc(nparams, sizeof(int));
      for (int i = 0; i < nparams; i++) {
        param_formats[i] = binary_params ? 1 : 0;
        struct fy_node *param_node = fy_node_sequence_get_by_index(params, i);
        switch (fy_node_get_type(param_node)) {
        case FYNT_MAPPING:
        case FYNT_SEQUENCE: {
          char *json = fy_emit_node_to_string(param_node, FYECF_MODE_JSON_ONELINE);
          values[i] = json;
          break;
        }
        case FYNT_SCALAR: {
          size_t len;
          values[i] = fy_node_get_scalar(param_node, &len);
          if (binary_params) {
            if (strncasecmp(values[i], "0x", 2) == 0) {
              size_t sz = (len - 2) / 2;
              char *binary_encoded_value = malloc(sz);
              for (size_t j = 0; j < len; j++) {
                sscanf(values[i] + 2 + 2 * j, "%2hhx", &binary_encoded_value[j]);
              }
              values[i] = binary_encoded_value;
              param_length[i] = sz;
            } else {
              // Use the supplied string as the source for binary
              // if it is not a hex
              param_length[i] = strlen(values[i]);
              // This way we can free it, too
              values[i] = strdup(values[i]);
            }
          } else {
            // This way we can free it, too
            values[i] = strdup(values[i]);
          }
          break;
        }
        default:
          // FIXME: handle unsupported type
          break;
        }
      }
      result = PQexecParams(conn, query, nparams, NULL, (const char **)values, param_length,
                            param_formats, result_format);

      if (binary_params) {
        // Free parsed values
        for (int i = 0; i < nparams; i++) {
          free((void *)values[i]);
        }
      }
      free((void *)values);
      free(param_formats);
    }

    ExecStatusType status = PQresultStatus(result);

    // FIXME: this block below is complicated and needs to be refactored.

    // Unless we're in a larger transaction,
    if (!in_transaction) {
      // attempt to finalize a transaction
      PGresult *txend_result = PQexec(conn, test->commit ? "commit" : "rollback");
      // if the query itself went fine,
      if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) {
        // check how transaction finalization went instead
        status = PQresultStatus(txend_result);
        // if transaction finalization didn't go well,
        if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
          // use its results to produce the error
          result = txend_result;
        }
        // otherwise, get results from the original query
      } else {
        PQclear(txend_result);
      }
    }

    // If it failed:
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
      // If current test is a scalar, change it to mapping to indicate
      // the error
      if (fy_node_is_scalar(test->node)) {
        struct fy_node *parent = fy_node_get_parent(test->node);
        assert(fy_node_is_sequence(parent));
        struct fy_node *new_node = fy_node_create_mapping(fy_node_document(parent));
        fy_node_set_meta(new_node, test);

        // TODO: This is not perfect
        // See https://github.com/pantoniou/libfyaml/issues/84
        fy_node_sequence_insert_after(parent, test->node, new_node);
        fy_node_sequence_remove(parent, test->node);

        // Add the query itself once it has been detached (otherwise this will error out)
        {
          int rc = fy_node_mapping_append(
              new_node, fy_node_create_scalar(fy_node_document(parent), STRLIT("query")),
              test->node);
          // This assertion helps ensuring that the above code is in the right place
          assert(rc == 0);
        }

        test->node = new_node;
      }

      // Change `success` to `false`
      if (errored != NULL) {
        *errored = true;
      }

      // Remove `results`
      struct fy_node *results_key =
          fy_node_mapping_lookup_key_by_string(test->node, STRLIT("results"));
      if (results_key != NULL) {
        fy_node_mapping_remove_by_key(test->node, results_key);
      }

      struct fy_node *error_key = fy_node_create_scalar(doc, STRLIT("error"));

      struct fy_node *existing_error = fy_node_mapping_lookup_value_by_key(test->node, error_key);

      // if `error` is a boolean
      if (existing_error != NULL && fy_node_is_boolean(existing_error)) {
        // Indicate that there is an error by setting error to true
        fy_node_mapping_remove_by_key(test->node, fy_node_copy(doc, error_key));
        fy_node_mapping_append(test->node, error_key, fy_node_create_scalar(doc, STRLIT("true")));
      } else {
        // Otherwise, specify the error
        char *errmsg = strdup(PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY));
        trim_trailing_whitespace(errmsg);

        struct fy_node *error = fy_node_create_scalar(doc, STRLIT(errmsg));

        fy_node_mapping_remove_by_key(test->node, fy_node_copy(doc, error_key));

        if (fy_node_is_scalar(existing_error)) {
          fy_node_mapping_append(test->node, error_key, error);
        } else {
          char *severity = strdup(PQresultErrorField(result, PG_DIAG_SEVERITY));
          struct fy_node *error_severity = fy_node_create_scalar(doc, STRLIT(severity));

          struct fy_node *error_node = fy_node_create_mapping(doc);
          fy_node_mapping_append(error_node, fy_node_create_scalar(doc, STRLIT("severity")),
                                 error_severity);
          fy_node_mapping_append(error_node, fy_node_create_scalar(doc, STRLIT("message")), error);

          // Add `detail` only if it is present
          if (fy_node_mapping_lookup_key_by_string(existing_error, STRLIT("detail")) != NULL) {
            char *maybe_detail = PQresultErrorField(result, PG_DIAG_MESSAGE_DETAIL);
            char *detail = maybe_detail ? strdup(maybe_detail) : "";
            struct fy_node *error_detail = fy_node_create_scalar(doc, STRLIT(detail));

            fy_node_mapping_append(error_node, fy_node_create_scalar(doc, STRLIT("detail")),
                                   error_detail);
          }

          fy_node_mapping_append(test->node, error_key, error_node);
        }
      }
    } else {
      struct fy_node *error_key = fy_node_create_scalar(doc, STRLIT("error"));

      // If `error` key is present, remove it
      if (fy_node_mapping_lookup_key_by_key(test->node, error_key) != NULL) {
        fy_node_mapping_remove_by_key(test->node, fy_node_copy(doc, error_key));
      }

      struct fy_node *results_key = fy_node_create_scalar(doc, STRLIT("results"));

      // If `results` are present, include them
      if (fy_node_mapping_lookup_key_by_key(test->node, results_key) != NULL) {

        struct fy_node *results = fy_node_create_sequence(doc);

        int ncolumns = PQnfields(result);

        for (int row = 0; row < PQntuples(result); row++) {
          struct fy_node *row_map = fy_node_create_mapping(doc);
          for (int column = 0; column < ncolumns; column++) {
            Oid column_type = PQftype(result, column);
            char *str_value =
                PQgetisnull(result, row, column) ? "null" : PQgetvalue(result, row, column);
            struct fy_node *value;
            // If results are returned as binary
            if (result_format == 1) {
              // output hexadecimal representation
              int len = PQgetlength(result, row, column);
              int sz = /* 0x */ 2 + /* FF */ len * 2 + /* null */ 1;
              char *hex = malloc(sz);
              char *ptr = hex;
              ptr += sprintf(ptr, "0x");
              for (int i = 0; i < len; i++) {
                ptr += sprintf(ptr, "%02x", str_value[i]);
              }
              // create a copy as we'll deallocate hex
              value = fy_node_create_scalar_copy(doc, hex, sz - 1);
              free(hex);
            } else {
              union fy_scalar_attributes attr = {.definitely_not_a_bool = true};
              if (column_type == test->instance->types.json ||
                  column_type == test->instance->types.jsonb) {
                value = fy_node_build_from_string(doc, STRLIT(str_value));
              } else if (column_type == test->instance->types.boolean) {
                value = strncmp(str_value, "t", 1) == 0
                            ? fy_node_create_scalar(doc, STRLIT("true"))
                            : fy_node_create_scalar(doc, STRLIT("false"));
                attr.definitely_not_a_bool = false;
              } else {
                value = fy_node_create_scalar(doc, STRLIT(str_value));
              }
              fy_node_set_meta(value, attr.ptr);
            }

            fy_node_mapping_append(
                row_map, fy_node_create_scalar(doc, STRLIT(PQfname(result, column))), value);
          }
          fy_node_sequence_append(results, row_map);
        }

        fy_node_mapping_remove_by_key(test->node, fy_node_copy(doc, results_key));
        fy_node_mapping_append(test->node, results_key, results);
      }
      // IMPORTANT:
      // We are not releasing (`PQclear(result)`) the results of the query as
      // those will be used to print the result YAML file.
    }

    break;
  }
  case ytest_kind_tests:
  case ytest_kind_steps: {
    struct fy_node *steps = fy_node_mapping_lookup_by_string(
        test->node, STRLIT(test->kind == ytest_kind_tests ? "tests" : "steps"));
    void *iter = NULL;
    struct fy_node *step;

    if ((test->kind == ytest_kind_tests && test->transaction) || !in_transaction) {
      PGresult *begin_result = PQexec(conn, "begin");
      PQclear(begin_result);
    }

    bool step_failed = false;

    taprintf("# Subtest: %.*s\n", (int)IOVEC_STRLIT(ytest_name(test)));
    int saved_tap_counter = tap_counter;
    tap_counter = 0;

    sub_test++;
    taprintf("1..%d\n", fy_node_sequence_item_count(steps));

    while ((step = fy_node_sequence_iterate(steps, &iter))) {
      ytest *y_test = (ytest *)fy_node_get_meta(step);
      // save notice receiver arg of parent test for multi-step tests
      y_test->prev_notices = notices;
      if (!step_failed) {
        bool errored = false;
        bool step_success = ytest_run_internal(
            conn, y_test, (test->kind == ytest_kind_tests && test->transaction) ? false : true,
            sub_test, &errored);

        // Stop proceeding further if the step has failed
        if (!step_success) {
          step_failed = true;
          // TODO: improve this
          // This is a compensation mechanism for the following behavior:
          // When a scalar test fails, it's replaced by a mapping with an error
          // However, because it does remove/insert for this replacement
          // (see https://github.com/pantoniou/libfyaml/issues/84)
          // `iter` still points to the removed node but not the new one
          // However, the code doing this updates `y_test->node`, so we'll use
          // that to ensure we're on the right element of the sequence.
          iter = (void *)y_test->node;
        } else {
          if (errored) {
            // Reset the transaction
            PGresult *txend_result = PQexec(conn, "rollback");
            PQclear(txend_result);

            // Start a new transaction
            PGresult *begin_result = PQexec(conn, "begin");
            PQclear(begin_result);
          } else if (!in_transaction && y_test->commit) {
            // Commit current transaction
            PGresult *txend_result = PQexec(conn, "commit");
            PQclear(txend_result);

            // Start a new transaction
            PGresult *begin_result = PQexec(conn, "begin");
            PQclear(begin_result);
          }
        }
      } else {
        // Skip tests after a failure
        tap_counter++;
        taprintf("ok %d - %.*s # SKIP\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(y_test)));
      }
    }

    sub_test--;

    tap_counter = saved_tap_counter;

    if ((test->kind == ytest_kind_tests && test->transaction) || !in_transaction) {
      PGresult *txend_result = PQexec(conn, test->commit ? "commit" : "rollback");
      PQclear(txend_result);
    }
    break;
  }
  case ytest_kind_restart: {
    PQfinish(conn);
    test->instance->conn = NULL;
    tap_counter++;
    taprintf("ok %d - %.*s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)));
    restart_instance(test->instance);
    return true;
  }
  default:
    break;
  }

  struct fy_node *notices_key = fy_node_create_scalar(doc, STRLIT("notices"));

  // If there are notices and `notices` key is declared
  if (!fy_node_sequence_is_empty(notices) &&
      fy_node_mapping_lookup_key_by_key(test->node, notices_key) != NULL) {
    // Replace `notices` with received notices
    fy_node_mapping_remove_by_key(test->node, fy_node_copy(doc, notices_key));
    fy_node_mapping_append(test->node, notices_key, notices);
  }

  PQsetNoticeReceiver(conn, prev_notice_receiver, test->prev_notices);

  // Handle `todo` tests
  struct fy_node *todo = fy_node_mapping_lookup_by_string(test->node, STRLIT("todo"));

  bool todo_allocated = false;
  iovec_t todo_reason = {.base = "", .len = 0};
  bool is_todo = false;
  if (todo != NULL) {
    if (fy_node_is_boolean(todo)) {
      if (!fy_node_get_boolean(todo)) {
        goto report;
      }
    } else if (fy_node_is_scalar(todo)) {
      todo_reason.base = fy_node_get_scalar(todo, &todo_reason.len);
    } else {
      todo_reason.base = fy_emit_node_to_string(todo, FYECF_NO_ENDING_NEWLINE | FYECF_WIDTH_INF |
                                                          FYECF_MODE_FLOW_ONELINE);
      todo_reason.len = strlen(todo_reason.base);
      todo_allocated = true;
    }
    is_todo = true;
  }

report:
  tap_counter++;
  bool success = false;
  bool equal =
      fy_node_compare_user(test->node, original_node, NULL, NULL, boolean_aware_cmp_fn, NULL);
  if ((success = equal == !test->negative)) {
    if (is_todo) {
      taprintf("ok %d - %.*s # TODO %*.s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)),
               (int)IOVEC_STRLIT(todo_reason));
    } else {
      taprintf("ok %d - %.*s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)));
    }
  } else {
    if (is_todo) {
      taprintf("not ok %d - %.*s # TODO %*.s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)),
               (int)IOVEC_STRLIT(todo_reason));
    } else {
      taprintf("not ok %d - %.*s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)));
    }
    // Unless it is a failure of a step, show the error
    if (test->kind != ytest_kind_steps) {
      taprintf("  ---\n");
      struct fy_document *original_doc = fy_node_document(original_node);
      struct fy_node *report = fy_node_create_mapping(original_doc);
      if (test->negative) {
        taprintf("  # negative expectation failed:\n");
        fy_node_mapping_append(report, fy_node_create_scalar(original_doc, STRLIT("expected")),
                               fy_node_copy(original_doc, original_node));
      } else {
        fy_node_mapping_append(report, fy_node_create_scalar(original_doc, STRLIT("expected")),
                               fy_node_copy(original_doc, original_node));
        fy_node_mapping_append(report, fy_node_create_scalar(original_doc, STRLIT("result")),
                               fy_node_copy(original_doc, test->node));
      }

      char *yaml_report = fy_emit_node_to_string(report, FYECF_DEFAULT);
      FILE *yamlf = fmemopen((void *)yaml_report, strlen(yaml_report), "r");

      char *line = NULL;
      size_t line_len = 0;

      while (getline(&line, &line_len, yamlf) != -1) {
        taprintf("  %.*s", (int)line_len, line);
      }

      free(line);
      fclose(yamlf);

      taprintf("\n  ...\n");
    }
  }
  fflush(tap_file);

  if (todo_allocated) {
    free((void *)todo_reason.base);
  }
  // free connection to other database because it is recreated each time
  if (test->instance->managed && test->database.base != NULL) {
    PQfinish(conn);
  }
  return is_todo ? true : success;
#undef taprintf
}

bool ytest_run(ytest *test) { return ytest_run_internal(NULL, test, false, 0, NULL); }
bool ytest_run_without_transaction(ytest *test, int sub_test) {
  // FIXME: this is only used for initializing instances and those create subtests,
  // that's why we use subtest indentation of 1. But the name of the function doesn't really reflect
  // any of this
  return ytest_run_internal(NULL, test, true, sub_test, NULL);
}

iovec_t ytest_name(ytest *test) {
  if (test->name.base != NULL) {
    return test->name;
  } else if (test->kind == ytest_kind_query) {
    char *query = strndup(test->info.query.query.base, test->info.query.query.len);
    // get rid of newlines and hashes
    for (char *str = query; *str != '\0'; ++str) {
      if (*str == '\n') {
        *str = ' ';
      }
      if (*str == '#') {
        *str = '%';
      }
    }
    return (iovec_t){.base = query, .len = test->info.query.query.len};
  } else {
    char *path = fy_node_get_path(test->node);
    return (iovec_t){.base = path, .len = strlen(path)};
  }
}