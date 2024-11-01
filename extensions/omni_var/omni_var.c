/**
 * @file omni_var.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <utils/guc.h>

PG_MODULE_MAGIC;

int num_scope_vars = 0;

void _PG_init() {
  DefineCustomIntVariable(
      "omni_var.estimated_initial_variables_count", "Estimated number of variables per scope",
      "Estimated number of transaction variables, changes apply to the next creation of the scope",
      &num_scope_vars, 1024, 0, 65535, PGC_USERSET, 0, NULL, NULL, NULL);
}
