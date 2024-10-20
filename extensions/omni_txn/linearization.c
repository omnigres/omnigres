// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/twophase.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <storage/predicate.h>
#include <tcop/pquery.h>
#include <utils/lsyscache.h>
#include <utils/snapmgr.h>
#if PG_MAJORVERSION_NUM < 17
#include <utils/typcache.h>
#endif
#if PG_MAJORVERSION_NUM < 15
#include <access/xact.h>
#endif
#include <nodes/pg_list.h>
#include <storage/predicate_internals.h>
#include <storage/proc.h>
#if PG_MAJORVERSION_NUM < 17
#define MyProcNumber (MyProc->pgprocno)
#endif
#include <utils/memutils.h>

#include <omni/omni_v0.h>

#include "omni_txn.h"

static bool initialized = false;
static bool linearize_enabled = false;

static List *linearized_writes;

typedef enum {
  Linearized_Inactive = 0,
  Linearized_In_Progress = 1,
  Linearized_Committing = 2,
} LinearizedBackendStatus;

#define MAX_CONFLICTS 1024

typedef struct {
  TransactionId txid;
  Oid rel;
} LinearizationConflict;

typedef struct {
  pid_t pid;
  LinearizedBackendStatus status;
  SERIALIZABLEXACT *xact;
  pg_atomic_uint32 numconflicts;
  LinearizationConflict conflicts[MAX_CONFLICTS];
} LinearizedBackend;

typedef struct {
  int MaxProcs;
  LinearizedBackend backends[FLEXIBLE_ARRAY_MEMBER];
} Control;

static Control *control;

#define __reset_local_control(control)                                                             \
  control->backends[MyProcNumber].xact = NULL;                                                     \
  control->backends[MyProcNumber].status = Linearized_Inactive;                                    \
  pg_atomic_write_u32(&control->backends[MyProcNumber].numconflicts, 0)

#define __reset_control() __reset_local_control(control)

static inline void check_if_rel_in_conflict(Oid rel) {
  uint32 numconflicts = pg_atomic_read_u32(&control->backends[MyProcNumber].numconflicts);
  for (int j = 0; j < numconflicts; j++) {
    if (control->backends[MyProcNumber].conflicts[j].rel == rel) {
      __reset_control();
      ereport(ERROR, errcode(ERRCODE_T_R_SERIALIZATION_FAILURE), errmsg("linearization failure"),
              errdetail("transaction %d has written to `%s` and have since committed",
                        control->backends[MyProcNumber].conflicts[j].txid, get_rel_name(rel)));
    }
  }
}

static void linearize_executor_start(omni_hook_handle *handle, QueryDesc *queryDesc, int eflags) {
  if (!linearize_enabled) {
    return;
  }
  if (control->backends[MyProcNumber].xact == NULL) {
    SERIALIZABLEXACT *xact = (SERIALIZABLEXACT *)ShareSerializableXact();
    if (xact != NULL) {
      control->backends[MyProcNumber].xact = xact;
      control->backends[MyProcNumber].status = Linearized_In_Progress;
    }
  }

  switch (queryDesc->operation) {
  case CMD_INSERT:
  case CMD_DELETE:
#if PG_MAJORVERSION_NUM > 14
  case CMD_MERGE:
#endif
  case CMD_UPDATE: {
    if (IsA(queryDesc->plannedstmt->planTree, ModifyTable)) {
      ModifyTable *node = castNode(ModifyTable, queryDesc->plannedstmt->planTree);

      PredicateLockData *locks = GetPredicateLockStatusData();
      ListCell *lc;
      foreach (lc, node->resultRelations) {
        int index = lfirst_int(lc);
        RangeTblEntry *entry =
            list_nth_node(RangeTblEntry, queryDesc->plannedstmt->rtable, index - 1);
        MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
        linearized_writes = list_append_unique_oid(linearized_writes, entry->relid);
        MemoryContextSwitchTo(oldcontext);
        for (int i = 0; i < locks->nelements; i++) {
          PREDICATELOCKTARGETTAG *tag = &(locks->locktags[i]);
          SERIALIZABLEXACT *xact = &(locks->xacts[i]);
          if (xact->pid == MyProcPid) {
            // Self-linearization failures are not possible
            continue;
          }

          Oid rel = GET_PREDICATELOCKTARGETTAG_RELATION(*tag);

          if (entry->relid == rel) {
            ereport(ERROR, errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
                    errmsg("linearization failure"),
                    errdetail("transaction %d has a predicate lock on `%s`", xact->topXid,
                              get_rel_name(rel)));
          }
        }
      }
    }
    break;
  }
  default: {
    ListCell *lc;
    foreach (lc, queryDesc->plannedstmt->rtable) {
      RangeTblEntry *entry = lfirst_node(RangeTblEntry, lc);
      check_if_rel_in_conflict(entry->relid);
    }
    break;
  }
  }
}

#if PG_MAJORVERSION_NUM > 14
#define SxactIsOurs(xact) xact->pgprocno == MyProcNumber
#else
#define SxactIsOurs(xact) xact->pid == MyProcPid
#endif

static void linearize_xact_callback(omni_hook_handle *handle, XactEvent event) {
  if (linearize_enabled && event == XACT_EVENT_PRE_COMMIT) {
    control->backends[MyProcNumber].status = Linearized_Committing;

    PredicateLockData *locks = GetPredicateLockStatusData();
    for (int i = 0; i < locks->nelements; i++) {
      PREDICATELOCKTARGETTAG *tag = &(locks->locktags[i]);
      SERIALIZABLEXACT *xact = &(locks->xacts[i]);

      Oid rel = GET_PREDICATELOCKTARGETTAG_RELATION(*tag);

      if (SxactIsOurs(xact)) {
        check_if_rel_in_conflict(rel);
      }
    }

    ListCell *lc;
    foreach (lc, linearized_writes) {
      Oid linearized_rel = lfirst_oid(lc);

      PredicateLockData *locks = GetPredicateLockStatusData();
      for (int i = 0; i < locks->nelements; i++) {
        PREDICATELOCKTARGETTAG *tag = &(locks->locktags[i]);
        SERIALIZABLEXACT *xact = &(locks->xacts[i]);

        Oid rel = GET_PREDICATELOCKTARGETTAG_RELATION(*tag);

        if (SxactIsOurs(xact)) {
          // Self-linearization failures are not possible
          continue;
        }
        if (rel == linearized_rel) {
          __reset_control();
          ereport(ERROR, errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
                  errmsg("linearization failure"),
                  errdetail("transaction %d has a predicate lock on `%s`", xact->topXid,
                            get_rel_name(rel)));
        }
      }

      LWLockAcquire(ProcArrayLock, LW_SHARED);
      for (int i = 0; i < control->MaxProcs; i++) {

        if (i == MyProcNumber) {
          // Skip ourselves
          continue;
        }

        PGPROC *proc = GetPGProcByNumber(i);

        if (control->backends[i].status == Linearized_In_Progress) {
          // FIXME: Every linearized transaction first starts as non-linearized one
          // and only then becomes linearized. What do we do about this gap? The transaction
          // may become linearized. In fact, the transaction might not even start as serializable
          // as that can be changed by `set transaction isolation level`
          // If the transaction has not taken a snapshot until we've committed, we should ignore it,
          // but not yet sure what's the best way to go about it.

          uint32 conflict_index = pg_atomic_fetch_add_u32(&control->backends[i].numconflicts, 1);
          if (conflict_index >= MAX_CONFLICTS) {
            __reset_control();
            ereport(ERROR, errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
                    errmsg("linearization failure"),
                    errdetail("transaction %d has too many potential conflicts to record",
                              control->backends[MyProcNumber].xact->topXid),
                    errhint("try again"));
          }
          control->backends[i].conflicts[conflict_index] = (LinearizationConflict){
              .txid = control->backends[MyProcNumber].xact->topXid, .rel = linearized_rel};
        }
      }
      LWLockRelease(ProcArrayLock);
    }
    __reset_control();
  }
  linearize_enabled = false;
}

static void init_control(const omni_handle *handle, void *ptr, void *data, bool allocated) {
  Control *c = (Control *)ptr;
  if (allocated) {
    c->MaxProcs = ProcGlobal->allProcCount + max_prepared_xacts;
  }
  c->backends[MyProcNumber].pid = MyProcPid;
  __reset_local_control(c);
}

void linearization_init(const omni_handle *handle) {
  omni_hook linearize_hook = {.name = "omni_txn linearize hook",
                              .type = omni_hook_executor_start,
                              .fn = {.executor_start = linearize_executor_start}};
  omni_hook xact_cleanup_hook = {.name = "omni_txn linearize cleanup hook",
                                 .type = omni_hook_xact_callback,
                                 .fn = {.xact_callback = linearize_xact_callback}};
  handle->register_hook(handle, &linearize_hook);
  handle->register_hook(handle, &xact_cleanup_hook);
  bool found;
  control = handle->allocate_shmem(
      handle, "omni_txn_linearization_control",
      sizeof(Control) + sizeof(LinearizedBackend) * (ProcGlobal->allProcCount + max_prepared_xacts),
      init_control, NULL, &found);
  initialized = true;
}

PG_FUNCTION_INFO_V1(linearize);

void linearize_transaction() {
  Assert(initialized);
  Assert(IsolationIsSerializable());
  control->backends[MyProcNumber].status = Linearized_In_Progress;
  linearize_enabled = true;
  linearized_writes = NIL;
}

Datum linearize(PG_FUNCTION_ARGS) {
  if (!initialized) {
    ereport(ERROR, errmsg("this functionality requires `omni` to be preloaded"));
  }
  if (!IsolationIsSerializable()) {
    ereport(ERROR, errmsg("current transaction is not serializable"));
  }

  linearize_transaction();

  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(linearized);

Datum linearized(PG_FUNCTION_ARGS) { PG_RETURN_BOOL(linearize_enabled); }