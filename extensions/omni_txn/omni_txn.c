// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <omni/omni_v0.h>

PG_MODULE_MAGIC;

OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni_txn", .version = EXT_VERSION,
                 .identity = "72b87e01-b6d4-440a-bcf0-d7a5ff8757f8");

#include "omni_txn.h"

void _Omni_init(const omni_handle *handle) { linearization_init(handle); }
