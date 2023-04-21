#include <assert.h>

#include <libpq-fe.h>

#include "pg_yregress.h"

void ytest_run(ytest *test) {
  assert(test->instance != NULL);
  if (!test->instance->ready) {
    yinstance_start(test->instance);
  }
  PGconn *conn = test->instance->conn;
  assert(conn != NULL);
  switch (test->kind) {
  case ytest_kind_query: {

    PGresult *begin_result = PQexec(conn, "BEGIN");
    PQclear(begin_result);

    // Prepare the query
    char *query;
    asprintf(&query, "%.*s", (int)IOVEC_STRLIT(test->info.query.query));
    // Execute the query
    PGresult *result = PQexec(conn, query);
    ExecStatusType status = PQresultStatus(result);

    // If it failed:
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
      // Change `success` to `false`
      fy_node_mapping_append(test->node, fy_node_create_scalar(test->doc, STRLIT("success")),
                             fy_node_create_scalar(test->doc, STRLIT("false")));

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
            struct fy_node *value =
                fy_node_create_scalar(test->doc, STRLIT(PQgetvalue(result, row, column)));

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

    PGresult *rollback_result = PQexec(conn, "ROLLBACK");
    PQclear(rollback_result);

    break;
  }
  default:
    break;
  }
}

iovec_t ytest_name(ytest *test) {
  if (test->name.base != NULL) {
    return test->name;
  } else {
    char *path = fy_node_get_path(test->node);
    return (iovec_t){.base = path, .len = strlen(path)};
  }
}