#ifndef OMNI_EXT_WORKERS_H
#define OMNI_EXT_WORKERS_H

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

extern HASHCTL worker_rendezvous_ctl;

/**
 * @brief Per-extension rendezvous point for database_worker
 *
 */
typedef struct {
  // Key, has to be first
  uint32 extension_hash;
  /**
   * @brief Has this extension been used in any database?
   *
   */
  bool used;
  /**
   * @brief PID of the datatabase_worker that will be responsible for
   *        starting global workers for this extension
   *
   */
  pid_t global_worker_starter;
} database_worker_rendezvous;

#endif // OMNI_EXT_WORKERS_H