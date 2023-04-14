/**
 * @file omni_seq.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/xlog.h>

PG_MODULE_MAGIC;

/**
 * Returns system identifier
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(system_identifier);

Datum system_identifier(PG_FUNCTION_ARGS) { PG_RETURN_INT64(GetSystemIdentifier()); }

#define PREFIX_TYPE int16
#define VAL_TYPE int16

#include "prefix_seq.h"

#define VAL_TYPE int32

#include "prefix_seq.h"

#define VAL_TYPE int64

#include "prefix_seq.h"

#undef PREFIX_TYPE

#define PREFIX_TYPE int32
#define VAL_TYPE int16

#include "prefix_seq.h"

#define VAL_TYPE int32

#include "prefix_seq.h"

#define VAL_TYPE int64

#include "prefix_seq.h"

#undef PREFIX_TYPE

#define PREFIX_TYPE int64
#define VAL_TYPE int16

#include "prefix_seq.h"

#define VAL_TYPE int32

#include "prefix_seq.h"

#define VAL_TYPE int64

#include "prefix_seq.h"