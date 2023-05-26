#include <assert.h>

#include <libpq-fe.h>

#include "pg_yregress.h"

static void notice_receiver(struct fy_node *notices, const PGresult *result) {
  char *notice = strdup(PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY));
  struct fy_document *doc = fy_node_document(notices);
  fy_node_sequence_append(notices, fy_node_create_scalar(doc, STRLIT(notice)));
}

bool ytest_run_internal(PGconn *default_conn, ytest *test, bool in_transaction, bool *errored) {

  // This will be used for TAP output
  struct fy_node *original_node = fy_node_copy(fy_node_document(test->node), test->node);

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
    fprintf(tap_file, "ok %d - %.*s # SKIP %.*s\n", tap_counter,
            (int)IOVEC_STRLIT(ytest_name(test)), (int)IOVEC_STRLIT(reason));
    if (allocated) {
      free((void *)reason.base);
    }
    return true;
  }

proceed:

  assert(test->instance != NULL);
  PGconn *conn;
  if (default_conn == NULL) {
    struct fy_node *test_container = fy_node_get_parent(test->node);
    assert(fy_node_is_sequence(test_container));
    struct fy_node *maybe_instance = fy_node_get_parent(test_container);
    // It is either a root or an instance
    assert(fy_node_is_mapping(maybe_instance));

    // If instance is not ready, start it
    // (unless this test is part of the initialization sequence)
    if (!test->instance->ready && maybe_instance != test->instance->node) {
      yinstance_start(test->instance);
      // If instance is still not ready, it means there was a recoverable error
      if (!test->instance->ready) {
        // We can't do much about it, bail.
        return false;
      }
    }
    // Ensure we always try to connect. It'll cache the connection
    // when reasonable
    yinstance_connect(test->instance);
    conn = test->instance->conn;
  } else {
    conn = default_conn;
  }
  assert(conn != NULL);

  // We will store notices here
  struct fy_node *notices = fy_node_create_sequence(fy_node_document(test->node));
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
      fy_node_mapping_append(test->node,
                             fy_node_create_scalar(fy_node_document(test->node), STRLIT("success")),
                             fy_node_create_scalar(fy_node_document(test->node), STRLIT("false")));

      fy_node_mapping_append(test->node,
                             fy_node_create_scalar(fy_node_document(test->node), STRLIT("success")),
                             fy_node_create_scalar(fy_node_document(test->node), STRLIT("false")));

      // Remove `results`
      struct fy_node *results_key =
          fy_node_mapping_lookup_key_by_string(test->node, STRLIT("results"));
      if (results_key != NULL) {
        fy_node_mapping_remove_by_key(test->node, results_key);
      }

      struct fy_node *error_key =
          fy_node_create_scalar(fy_node_document(test->node), STRLIT("error"));

      // If `error` is present, replace it
      struct fy_node *existing_error = fy_node_mapping_lookup_value_by_key(test->node, error_key);
      if (existing_error != NULL) {
        char *errmsg = strdup(PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY));
        trim_trailing_whitespace(errmsg);

        struct fy_node *error = fy_node_create_scalar(fy_node_document(test->node), STRLIT(errmsg));

        fy_node_mapping_remove_by_key(test->node,
                                      fy_node_copy(fy_node_document(test->node), error_key));

        if (fy_node_is_scalar(existing_error)) {
          fy_node_mapping_append(test->node, error_key, error);
        } else {
          char *severity = strdup(PQresultErrorField(result, PG_DIAG_SEVERITY));
          struct fy_node *error_severity =
              fy_node_create_scalar(fy_node_document(test->node), STRLIT(severity));

          struct fy_node *error_node = fy_node_create_mapping(fy_node_document(test->node));
          fy_node_mapping_append(
              error_node, fy_node_create_scalar(fy_node_document(test->node), STRLIT("severity")),
              error_severity);
          fy_node_mapping_append(
              error_node, fy_node_create_scalar(fy_node_document(test->node), STRLIT("message")),
              error);

          // Add `detail` only if it is present
          if (fy_node_mapping_lookup_key_by_string(existing_error, STRLIT("detail")) != NULL) {
            char *maybe_detail = PQresultErrorField(result, PG_DIAG_MESSAGE_DETAIL);
            char *detail = maybe_detail ? strdup(maybe_detail) : "";
            struct fy_node *error_detail =
                fy_node_create_scalar(fy_node_document(test->node), STRLIT(detail));

            fy_node_mapping_append(
                error_node, fy_node_create_scalar(fy_node_document(test->node), STRLIT("detail")),
                error_detail);
          }

          fy_node_mapping_append(test->node, error_key, error_node);
        }
      }
    } else {
      struct fy_node *results_key =
          fy_node_create_scalar(fy_node_document(test->node), STRLIT("results"));

      // If `results` are present, include them
      if (fy_node_mapping_lookup_key_by_key(test->node, results_key) != NULL) {

        struct fy_node *results = fy_node_create_sequence(fy_node_document(test->node));

        int ncolumns = PQnfields(result);

        for (int row = 0; row < PQntuples(result); row++) {
          struct fy_node *row_map = fy_node_create_mapping(fy_node_document(test->node));
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
              value = fy_node_create_scalar_copy(fy_node_document(test->node), hex, sz - 1);
              free(hex);
            } else {
              if (column_type == test->instance->types.json ||
                  column_type == test->instance->types.jsonb) {
                value = fy_node_build_from_string(fy_node_document(test->node), STRLIT(str_value));
              } else {
                value = fy_node_create_scalar(fy_node_document(test->node), STRLIT(str_value));
              }
            }

            fy_node_mapping_append(row_map,
                                   fy_node_create_scalar(fy_node_document(test->node),
                                                         STRLIT(PQfname(result, column))),
                                   value);
          }
          fy_node_sequence_append(results, row_map);
        }

        fy_node_mapping_remove_by_key(test->node,
                                      fy_node_copy(fy_node_document(test->node), results_key));
        fy_node_mapping_append(test->node, results_key, results);
      }
    }

    // IMPORTANT:
    // We are not releasing (`PQclear(result)`) the results of the query as
    // those will be used to print the result YAML file.

    if (!in_transaction) {
      PGresult *txend_result = PQexec(conn, test->commit ? "commit" : "rollback");
      PQclear(txend_result);
    }

    break;
  }
  case ytest_kind_steps: {
    struct fy_node *steps = fy_node_mapping_lookup_by_string(test->node, STRLIT("steps"));
    void *iter = NULL;
    struct fy_node *step;

    if (!in_transaction) {
      PGresult *begin_result = PQexec(conn, "begin");
      PQclear(begin_result);
    }

    bool step_failed = false;

    while ((step = fy_node_sequence_iterate(steps, &iter))) {
      ytest *y_test = (ytest *)fy_node_get_meta(step);
      if (!step_failed) {
        bool errored = false;
        bool step_success = ytest_run_internal(conn, y_test, true, &errored);

        // Stop proceeding further if the step has failed
        if (!step_success) {
          step_failed = true;
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
        tap_counter++;
        fprintf(tap_file, "ok %d - %.*s # SKIP\n", tap_counter,
                (int)IOVEC_STRLIT(ytest_name(y_test)));
      }
    }

    if (!in_transaction) {
      PGresult *txend_result = PQexec(conn, test->commit ? "commit" : "rollback");
      PQclear(txend_result);
    }
    break;
  }
  case ytest_kind_restart: {
    PQfinish(conn);
    test->instance->conn = NULL;
    restart_instance(test->instance);
    return true;
  }
  default:
    break;
  }

  struct fy_node *notices_key =
      fy_node_create_scalar(fy_node_document(test->node), STRLIT("notices"));

  // If there are notices and `notices` key is declared
  if (!fy_node_sequence_is_empty(notices) &&
      fy_node_mapping_lookup_key_by_key(test->node, notices_key) != NULL) {
    // Replace `notices` with received notices
    fy_node_mapping_remove_by_key(test->node,
                                  fy_node_copy(fy_node_document(test->node), notices_key));
    fy_node_mapping_append(test->node, notices_key, notices);
  }

  PQsetNoticeReceiver(conn, prev_notice_receiver, NULL);

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
  bool differ = false;
  if ((differ = fy_node_compare(test->node, original_node))) {
    if (is_todo) {
      fprintf(tap_file, "ok %d - %.*s # TODO %*.s\n", tap_counter,
              (int)IOVEC_STRLIT(ytest_name(test)), (int)IOVEC_STRLIT(todo_reason));
    } else {
      fprintf(tap_file, "ok %d - %.*s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)));
    }
  } else {
    if (is_todo) {
      fprintf(tap_file, "not ok %d - %.*s # TODO %*.s\n", tap_counter,
              (int)IOVEC_STRLIT(ytest_name(test)), (int)IOVEC_STRLIT(todo_reason));
    } else {
      fprintf(tap_file, "not ok %d - %.*s\n", tap_counter, (int)IOVEC_STRLIT(ytest_name(test)));
    }
    fprintf(tap_file, "  ---\n");
    struct fy_node *report = fy_node_create_mapping(fy_node_document(original_node));
    fy_node_mapping_append(report,
                           fy_node_create_scalar(fy_node_document(report), STRLIT("expected")),
                           fy_node_copy(fy_node_document(original_node), original_node));
    fy_node_mapping_append(report,
                           fy_node_create_scalar(fy_node_document(report), STRLIT("result")),
                           fy_node_copy(fy_node_document(original_node), test->node));

    char *yaml_report = fy_emit_node_to_string(report, FYECF_DEFAULT);
    FILE *yamlf = fmemopen((void *)yaml_report, strlen(yaml_report), "r");

    char *line = NULL;
    size_t line_len = 0;

    while (getline(&line, &line_len, yamlf) != -1) {
      fprintf(tap_file, "  %.*s", (int)line_len, line);
    }

    free(line);
    fclose(yamlf);

    fprintf(tap_file, "\n  ...\n");
  }
  fflush(tap_file);

  if (todo_allocated) {
    free((void *)todo_reason.base);
  }

  return is_todo ? true : differ;
}

bool ytest_run(ytest *test) { return ytest_run_internal(NULL, test, false, NULL); }
void ytest_run_without_transaction(ytest *test) { ytest_run_internal(NULL, test, true, NULL); }

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