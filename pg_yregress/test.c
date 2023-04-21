#include <assert.h>

#include <libpq-fe.h>

#include "pg_yregress.h"

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

    struct fy_node *params = fy_node_mapping_lookup_by_string(test->node, STRLIT("params"));

    if (params == NULL) {
      result = PQexec(conn, query);
    } else if (fy_node_is_sequence(params)) {
      int nparams = fy_node_sequence_item_count(params);
      const char **values = calloc(nparams, sizeof(char *));
      for (int i = 0; i < nparams; i++) {
        struct fy_node *param_node = fy_node_sequence_get_by_index(params, i);
        switch (fy_node_get_type(param_node)) {
        case FYNT_SCALAR:
          values[i] = fy_node_get_scalar(param_node, NULL);
          break;
        default:
          // FXIME: handle unsupported type
          break;
        }
      }
      result = PQexecParams(conn, query, nparams, NULL, (const char **)values, NULL, NULL, 0);
    }

    ExecStatusType status = PQresultStatus(result);

    // If it failed:
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
      success = false;
      // Change `success` to `false`
      fy_node_mapping_append(test->node, fy_node_create_scalar(test->doc, STRLIT("success")),
                             fy_node_create_scalar(test->doc, STRLIT("false")));

      fy_node_mapping_append(test->node, fy_node_create_scalar(test->doc, STRLIT("success")),
                             fy_node_create_scalar(test->doc, STRLIT("false")));

      // Remove `results`
      struct fy_node *results_key =
          fy_node_mapping_lookup_key_by_string(test->node, STRLIT("results"));
      if (results_key != NULL) {
        fy_node_mapping_remove_by_key(test->node, results_key);
      }

      struct fy_node *error_key = fy_node_create_scalar(test->doc, STRLIT("error"));

      // If `error` is present, replace it
      if (fy_node_mapping_lookup_key_by_key(test->node, error_key) != NULL) {
        char *errmsg = strdup(PQresultErrorField(result, PG_DIAG_MESSAGE_PRIMARY));
        trim_trailing_whitespace(errmsg);
        char *severity = strdup(PQresultErrorField(result, PG_DIAG_SEVERITY));

        struct fy_node *error = fy_node_create_scalar(test->doc, STRLIT(errmsg));
        struct fy_node *error_severity = fy_node_create_scalar(test->doc, STRLIT(severity));
        fy_node_mapping_remove_by_key(test->node, fy_node_copy(test->doc, error_key));

        struct fy_node *error_node = fy_node_create_mapping(test->doc);
        fy_node_mapping_append(error_node, fy_node_create_scalar(test->doc, STRLIT("severity")),
                               error_severity);
        fy_node_mapping_append(error_node, fy_node_create_scalar(test->doc, STRLIT("message")),
                               error);

        fy_node_mapping_append(test->node, error_key, error_node);
      }
    } else {
      struct fy_node *results_key = fy_node_create_scalar(test->doc, STRLIT("results"));

      // If `results` are present, include them
      if (fy_node_mapping_lookup_key_by_key(test->node, results_key) != NULL) {

        struct fy_node *results = fy_node_create_sequence(test->doc);

        int ncolumns = PQnfields(result);

        for (int row = 0; row < PQntuples(result); row++) {
          struct fy_node *row_map = fy_node_create_mapping(test->doc);
          for (int column = 0; column < ncolumns; column++) {
            char *str_value =
                PQgetisnull(result, row, column) ? "null" : PQgetvalue(result, row, column);
            struct fy_node *value = fy_node_create_scalar(test->doc, STRLIT(str_value));

            fy_node_mapping_append(
                row_map, fy_node_create_scalar(test->doc, STRLIT(PQfname(result, column))), value);
          }
          fy_node_sequence_append(results, row_map);
        }

        fy_node_mapping_remove_by_key(test->node, fy_node_copy(test->doc, results_key));
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