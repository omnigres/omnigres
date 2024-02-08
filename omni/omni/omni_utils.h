#ifndef OMNI_UTILS_H
#define OMNI_UTILS_H

#include <dlfcn.h>
#include <stddef.h>

// clang-format off
#include <postgres.h>
#include <postmaster/bgworker.h>
// clang-format on
#include <access/xact.h>
#include <executor/executor.h>
#include <libpq/crypt.h>
#include <nodes/pathnodes.h>
#include <nodes/plannodes.h>
#include <tcop/utility.h>
#include <utils/guc.h>
#include <utils/guc_tables.h>

#if PG_MAJORVERSION_NUM >= 13 && PG_MAJORVERSION_NUM <= 16
/**
 * Omni's copy of an otherwise private datatype of `BackgroundWorkerHnalde`
 * so that it can be copied between backends.
 */
typedef struct {
  int slot;
  uint64 generation;
} OmniBackgroundWorkerHandle;

/**
 * Wrapper for `RegisterDynamicBackgroundWorker` to get `OmniBackgroundWorkerHandle`
 * instead of dynamically allocated `BackgroundWorkerHandle`.
 *
 * @param worker background worker definition
 * @param handle must be a non-NULL pointer to `OmniBackgroundWorkerHandle`
 */
static inline bool OmniRegisterDynamicBackgroundWorker(BackgroundWorker *worker,
                                                       OmniBackgroundWorkerHandle *handle) {
  BackgroundWorkerHandle *bgw_handle;
  bool result = RegisterDynamicBackgroundWorker(worker, &bgw_handle);
  memcpy(handle, bgw_handle, sizeof(OmniBackgroundWorkerHandle));
  // we must free `bgw_handle` because the memory context we're in may be long-living
  pfree(bgw_handle);
  return result;
}

/**
 * @brief Convert `OmniBackgroundWorkerHandle` to `BackgroundWorkerHandle`
 */
static inline BackgroundWorkerHandle *
GetBackgroundWorkerHandle(OmniBackgroundWorkerHandle *handle) {
  return (BackgroundWorkerHandle *)handle;
}

#else
#error "Ensure this version of Postgres is compatible with the above definition or use a new one"
#endif

#endif // OMNI_UTILS_H