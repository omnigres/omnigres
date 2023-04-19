/**
 * @file unit.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/heapam.h>
#include <access/htup_details.h>
#include <access/table.h>
#include <catalog/namespace.h>
#include <catalog/pg_cast.h>
#include <catalog/pg_collation.h>
#include <catalog/pg_language.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>
#include <commands/typecmds.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <utils/array.h>
#include <utils/builtins.h>
#include <utils/fmgroids.h>
#include <utils/lsyscache.h>
#include <utils/syscache.h>

/**
 * Converts cstring into a unit type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(unit_in);
/**
 * Converts unit type to a cstring
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(unit_out);

/**
 * Converts internal into a unit type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(unit_recv);
/**
 * Converts unit type to a byte array
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(unit_send);

/**
 * Makes unit type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(unit);

Datum unit_in(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  PG_RETURN_CHAR(0);
}

Datum unit_out(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  PG_RETURN_CSTRING("");
}

Datum unit_recv(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  PG_RETURN_CHAR(0);
}

Datum unit_send(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  static varattrib_1b bytearr = {
      .va_header = 1 << 1 | 0x01 // 0
  };
  PG_RETURN_POINTER(&bytearr);
}

Datum unit(PG_FUNCTION_ARGS) { PG_RETURN_CHAR(0); }
