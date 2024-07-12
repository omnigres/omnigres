#include <limits.h>

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <commands/dbcommands.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <storage/latch.h>
#include <storage/lwlock.h>
#include <utils/builtins.h>
#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include <omni/omni_v0.h>

#include "hooks.h"

PG_MODULE_MAGIC;
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "no_fun", .version = EXT_VERSION,
                 .identity = "18d79a47-d48d-489d-b7d5-c414a9ccb0d1");

void _Omni_init(const omni_handle *handle) {
  SPI_connect();
  SPI_execute("create table if not exists public.no_fun ()", false, 0);
  SPI_finish();
}
