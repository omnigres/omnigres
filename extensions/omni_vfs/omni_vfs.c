/**
 * @file omni_vfs.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <catalog/pg_enum.h>
#include <executor/spi.h>
#include <utils/syscache.h>

PG_MODULE_MAGIC;

#include "libpgaug.h"

CACHED_OID(omni_vfs_types_v1, file_kind);
CACHED_OID(omni_vfs_types_v1, file_info);
CACHED_ENUM_OID(file_kind, file)
CACHED_ENUM_OID(file_kind, dir)
