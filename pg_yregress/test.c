#include <assert.h>

#include <libpq-fe.h>

#include "pg_yregress.h"

static void notice_receiver(struct fy_node *notices, const PGresult *result) {
  char *notice = strdup(PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY));
  struct fy_document *doc = fy_node_document(notices);
  fy_node_sequence_append(notices, fy_node_create_scalar(doc, STRLIT(notice)));
}

bool ytest_run_internal(PGconn *default_conn, ytest *test, bool in_transaction) {
  bool success = true;
  assert(test->instance != NULL);
  PGconn *conn;
  if (default_conn == NULL) {
    if (!test->instance->ready) {
      yinstance_start(test->instance);
    }
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
      success = false;
      // Change `success` to `false`
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
      if (fy_node_mapping_lookup_key_by_key(test->node, error_key) != NULL) {
        char *errmsg = strdup(PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY));
        trim_trailing_whitespace(errmsg);
        char *severity = strdup(PQresultErrorField(result, PG_DIAG_SEVERITY));

        struct fy_node *error = fy_node_create_scalar(fy_node_document(test->node), STRLIT(errmsg));
        struct fy_node *error_severity =
            fy_node_create_scalar(fy_node_document(test->node), STRLIT(severity));
        fy_node_mapping_remove_by_key(test->node,
                                      fy_node_copy(fy_node_document(test->node), error_key));

        struct fy_node *error_node = fy_node_create_mapping(fy_node_document(test->node));
        fy_node_mapping_append(
            error_node, fy_node_create_scalar(fy_node_document(test->node), STRLIT("severity")),
            error_severity);
        fy_node_mapping_append(
            error_node, fy_node_create_scalar(fy_node_document(test->node), STRLIT("message")),
            error);

        fy_node_mapping_append(test->node, error_key, error_node);
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
              value = fy_node_create_scalar(fy_node_document(test->node), STRLIT(str_value));
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
      PGresult *rollback_result = PQexec(conn, "rollback");
      PQclear(rollback_result);
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

    while ((step = fy_node_sequence_iterate(steps, &iter))) {
      bool step_success = ytest_run_internal(conn, (ytest *)fy_node_get_meta(step), true);
      // Stop proceeding further if the step has failed
      if (!step_success) {
        break;
      }
    }

    if (!in_transaction) {
      PGresult *rollback_result = PQexec(conn, "rollback");
      PQclear(rollback_result);
    }
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
  return success;
}

void ytest_run(ytest *test) { ytest_run_internal(NULL, test, false); }

iovec_t ytest_name(ytest *test) {
  if (test->name.base != NULL) {
    return test->name;
  } else {
    char *path = fy_node_get_path(test->node);
    return (iovec_t){.base = path, .len = strlen(path)};
  }
}