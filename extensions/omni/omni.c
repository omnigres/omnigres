// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/heapam.h>
#include <access/table.h>
#include <catalog/pg_extension.h>
#include <catalog/pg_language.h>
#include <catalog/pg_proc.h>
#include <commands/user.h>
#include <executor/executor.h>
#include <miscadmin.h>
#include <storage/latch.h>
#include <storage/lwlock.h>
#include <utils/builtins.h>
#include <utils/hsearch.h>
#include <utils/memutils.h>
#include <utils/rel.h>
#include <utils/syscache.h>
#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include "omni_common.h"

#include "dshash_extras.h"

PG_MODULE_MAGIC;
OMNI_MAGIC;

OMNI_MODULE_INFO(.name = "omni", .version = EXT_VERSION,
                 .identity = "d71344f3-7e9f-4987-9ebb-7fd0d9253157");

omni_shared_info *shared_info;

MODULE_VARIABLE(dshash_table *omni_modules);
MODULE_VARIABLE(dshash_table *omni_allocations);
MODULE_VARIABLE(HTAB *dsa_handles);

MODULE_VARIABLE(omni_handle_private *module_handles);

MODULE_VARIABLE(LWLockPadded *locks);

MODULE_VARIABLE(List *initialized_modules);

MODULE_VARIABLE(int OMNI_DSA_TRANCHE);

MODULE_VARIABLE(bool backend_force_reload);

MODULE_VARIABLE(MemoryContext OmniGUCContext);

static dsa_area *dsa = NULL;

MODULE_FUNCTION dsa_area *dsa_handle_to_area(dsa_handle handle) {
  if (dsa == NULL || (dsa != NULL && handle != dsa_get_handle(dsa))) {
    if (!dsm_find_mapping(handle)) {
      // It is important to allocate DSA in the top memory context
      // so it doesn't get deallocated when the context we're in is gone
      MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
      dsa_area *other_dsa = dsa_attach(handle);
      dsa_pin_mapping(other_dsa);
      MemoryContextSwitchTo(oldcontext);
      bool found = false;
      DSAHandleEntry *handle_entry =
          (DSAHandleEntry *)hash_search(dsa_handles, &handle, HASH_ENTER, &found);
      Assert(!found);
      handle_entry->dsa = other_dsa;
      return other_dsa;
    } else {
      bool found = false;
      DSAHandleEntry *handle_entry =
          (DSAHandleEntry *)hash_search(dsa_handles, &handle, HASH_ENTER, &found);
      Assert(found);
      return handle_entry->dsa;
    }
  } else {
    return dsa;
  }
}

static inline void initialize_omni_modules() {
  const dshash_parameters module_params = {
    .key_size = PATH_MAX,
    .entry_size = sizeof(ModuleEntry),
    .hash_function = dshash_string_hash,
    .compare_function = dshash_strcmp,
#if PG_MAJORVERSION_NUM >= 17
    .copy_function = dshash_string_copy,
#endif
    .tranche_id = OMNI_DSA_TRANCHE
  };
  const dshash_parameters allocation_params = {
    .key_size = sizeof(ModuleAllocationKey),
    .entry_size = sizeof(ModuleAllocation),
    .hash_function = dshash_memhash,
    .compare_function = dshash_memcmp,
#if PG_MAJORVERSION_NUM >= 17
    .copy_function = dshash_string_copy,
#endif
    .tranche_id = OMNI_DSA_TRANCHE
  };
  MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
  LWLockAcquire(&(locks + OMNI_LOCK_MODULE)->lock, LW_EXCLUSIVE);
  if (pg_atomic_test_set_flag(&shared_info->tables_initialized)) {
    // Nobody has initialized modules table yet. And we're under an exclusive lock,
    // so we can do it
    dsa_area *dsa = dsa_handle_to_area(shared_info->dsa);
    omni_modules = dshash_create(dsa, &module_params, NULL);
    shared_info->modules_tab = dshash_get_hash_table_handle(omni_modules);
    omni_allocations = dshash_create(dsa, &allocation_params, NULL);
    shared_info->allocations_tab = dshash_get_hash_table_handle(omni_allocations);
  } else {
    // Otherwise, attach to it
    dsa_area *dsa_area = dsa_handle_to_area(shared_info->dsa);
    omni_modules = dshash_attach(dsa_area, &module_params, shared_info->modules_tab, NULL);
    omni_allocations =
        dshash_attach(dsa_area, &allocation_params, shared_info->allocations_tab, NULL);
  }
  LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);
  MemoryContextSwitchTo(oldcontext);
}

static bool backend_initialized = false;

// List of `ModuleAllocationKey` entries specific to this backend
static List *backend_shmem_acquisitions = NIL;

static MemoryContext BackendModuleContextForModule(omni_handle_private *phandle) {
  for (MemoryContext cxt = TopMemoryContext->firstchild; cxt != NULL; cxt = cxt->nextchild) {
    if (strcmp(cxt->name, "TopMemoryContext/omni") == 0 && strcmp(cxt->ident, phandle->path) == 0) {
      return cxt;
    }
  }
  return NULL;
}

static bool ensure_backend_initialized(void) {
  // Indicates whether we have ever done anything on this backend
  if (backend_initialized)
    return true;

  if (!(MyBackendType == B_BACKEND || MyBackendType == B_BG_WORKER)) {
    return false;
  }

  // If we're out of any transaction (for example, `ShutdownPostgres` would do that), bail
  if (!IsTransactionState()) {
    return false;
  }

  LWLockRegisterTranche(OMNI_DSA_TRANCHE, "omni:dsa");

  locks = GetNamedLWLockTranche("omni");
  LWLockAcquire(&(locks + OMNI_LOCK_DSA)->lock, LW_EXCLUSIVE);

  if (pg_atomic_test_set_flag(&shared_info->dsa_initialized)) {
    // It is important to allocate DSA in the top memory context
    // so it doesn't get deallocated when the context we're in is gone
    MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    dsa = dsa_create(OMNI_DSA_TRANCHE);
    MemoryContextSwitchTo(oldcontext);
    dsa_pin(dsa);
    dsa_pin_mapping(dsa);
    shared_info->dsa = dsa_get_handle(dsa);
  }

  LWLockRelease(&(locks + OMNI_LOCK_DSA)->lock);

  {
    HASHCTL ctl = {.keysize = sizeof(dsa_handle), .entrysize = sizeof(DSAHandleEntry)};
    dsa_handles = hash_create("omni:dsa_handles", 128, &ctl, HASH_ELEM | HASH_BLOBS);
  }

  initialize_omni_modules();

  backend_initialized = true;
  initialized_modules = NIL;

  return true;
}

static void register_hook(const omni_handle *handle, omni_hook *hook);
static void *allocate_shmem(const omni_handle *handle, const char *name, size_t size,
                            omni_allocate_shmem_callback_function init, void *data, bool *found);
static void *allocate_shmem_0_0(const omni_handle *handle, const char *name, size_t size,
                                bool *found) {
  return allocate_shmem(handle, name, size, NULL, NULL, found);
}
static void deallocate_shmem(const omni_handle *handle, const char *name, bool *found);
static void *lookup_shmem(const omni_handle *handle, const char *name, bool *found);
static void declare_guc_variable(const omni_handle *handle, omni_guc_variable *variable);

static void request_bgworker_start(const omni_handle *handle, BackgroundWorker *bgworker,
                                   omni_bgworker_handle *bgworker_handle,
                                   const omni_bgworker_options options);

static void request_bgworker_termination(const omni_handle *handle,
                                         omni_bgworker_handle *bgworker_handle,
                                         const omni_bgworker_options options);

static void register_lwlock(const omni_handle *handle, LWLock *lock, const char *name,
                            bool initialize) {
  // TODO: unused tranche id reuse
  int tranche_id = initialize ? LWLockNewTrancheId() : lock->tranche;
  LWLockRegisterTranche(tranche_id, name);

  if (initialize) {
    LWLockInitialize(lock, tranche_id);
  }
}

#if PG_MAJORVERSION_NUM >= 13 && PG_MAJORVERSION_NUM <= 17
#define LW_FLAG_HAS_WAITERS ((uint32)1 << 30)
#define LW_FLAG_RELEASE_OK ((uint32)1 << 29)
#define LW_FLAG_LOCKED ((uint32)1 << 28)
#else
#error "Check these constants for this version of Postgres"
#endif

static void unregister_lwlock(const omni_handle *handle, LWLock *lock) {
  uint32 state = pg_atomic_read_u32(&lock->state);
  if ((state & LW_FLAG_RELEASE_OK) == 0) {
    ereport(ERROR, errmsg("lock can't be unregistered"),
            errdetail("it is not marked as ok to release"));
  }
  // TODO: schedule tranche id for reuse
}

static uint64 atomic_switch(const omni_handle *handle, omni_switch_operation op, uint32 switchboard,
                            uint64 mask) {
  if (switchboard > 0) {
    ereport(ERROR, errcode(ERRCODE_INSUFFICIENT_RESOURCES),
            errmsg("no more switchboards can be allocated"),
            errdetail("Current implementation only provides a single switchboard (0)"));
  }
  omni_handle_private *phandle = struct_from_member(omni_handle_private, handle, handle);
  switch (op) {
  case omni_switch_on: {
    uint64 m = pg_atomic_fetch_or_u64(&phandle->switchboard, mask);
    return (m ^ mask) & mask;
  }
  case omni_switch_off:
    return pg_atomic_fetch_and_u64(&phandle->switchboard, ~mask) & mask;
  }
}

/*
 * Substitute for any macros appearing in the given string.
 * Result is always freshly palloc'd.
 */
static char *substitute_libpath_macro(const char *name) {
  const char *sep_ptr;

  Assert(name != NULL);

  /* Currently, we only recognize $libdir at the start of the string */
  if (name[0] != '$')
    return pstrdup(name);

  if ((sep_ptr = first_dir_separator(name)) == NULL)
    sep_ptr = name + strlen(name);

  if (strlen("$libdir") != sep_ptr - name || strncmp(name, "$libdir", strlen("$libdir")) != 0)
    ereport(ERROR, (errcode(ERRCODE_INVALID_NAME),
                    errmsg("invalid macro name in dynamic library path: %s", name)));

  return psprintf("%s%s", pkglib_path, sep_ptr);
}

static omni_handle_private *load_module(const char *path,
                                        bool warning_on_omni_mismatch_preference) {
  omni_handle_private *result = NULL;
  void *dlhandle = dlopen(path, RTLD_LAZY);

  if (dlhandle != NULL) {
    // If loaded, check for magic header
    omni_magic *(*magic_fn)() = dlsym(dlhandle, "_Omni_magic");
    if (magic_fn != NULL) {
      // Check if magic is correct
      omni_magic *magic = magic_fn();

      omni_module_information *module_info = dlsym(dlhandle, "_omni_module_information");

      {
        // Omni compatibility check

#if PG_MAJORVERSION_NUM >= 17
        bool warn_on_mismatch = AmBackgroundWorkerProcess() || warning_on_omni_mismatch_preference;
#else
        bool warn_on_mismatch = IsBackgroundWorker || warning_on_omni_mismatch_preference;
#endif

        {
          // Special case for omni 0.1.0 (TODO: deprecate)
          void *database_worker_fn = dlsym(dlhandle, "database_worker");
          void *startup_worker_fn = dlsym(dlhandle, "startup_worker");
          void *deinitialize_backend_fn = dlsym(dlhandle, "deinitialize_backend");
          if (magic != NULL && magic->revision < 6 && module_info == NULL &&
              database_worker_fn != NULL && startup_worker_fn != NULL &&
              deinitialize_backend_fn != NULL && _Omni_magic != magic_fn) {
            ereport(warn_on_mismatch ? WARNING : ERROR,
                    errmsg("omni extension 0.1.0 is incompatible with a preloaded omni "
                           "library of %s, please upgrade",
                           _omni_module_information.version));
            return NULL;
          }
        }

        {
          if (magic != NULL && module_info != NULL &&
              strcmp(module_info->identity, _omni_module_information.identity) == 0 &&
              _Omni_magic != magic_fn) {

            // Check versions
            if (strcmp(module_info->version, _omni_module_information.version) != 0) {
              ereport(warn_on_mismatch ? WARNING : ERROR,
                      errmsg("omni extension %s is incompatible with a preloaded omni "
                             "library of %s",
                             module_info->version, _omni_module_information.version));
            }

            // Different file paths
            if (strcmp(path, get_omni_library_name()) != 0) {
              ereport(warn_on_mismatch ? WARNING : ERROR,
                      errmsg("attempting to loading omni extension from a file different from the "
                             "preloaded library"),
                      errdetail("expected %s, got %s", get_omni_library_name(), path));
            }

            // In this case, the path is the same, but we still didn't load into the same address
            // space
            ereport(warn_on_mismatch ? WARNING : ERROR,
                    errmsg("attempting to loading omni extension from a file different from the "
                           "preloaded library"));

            return NULL;
          }
        }
      }

      if (magic->size == sizeof(omni_magic) && magic->version == OMNI_INTERFACE_VERSION) {
        // We are going to record it if it wasn't yet
        LWLockAcquire(&(locks + OMNI_LOCK_MODULE)->lock, LW_EXCLUSIVE);
        bool found = false;
        ModuleEntry *entry = dshash_find_or_insert(omni_modules, path, &found);
        //
        omni_handle_private *handle;

        if (!found) {
          uint32 id = pg_atomic_add_fetch_u32(&shared_info->module_counter, 1);

          // If not found, prepare the handle
          dsa_area *dsa = dsa_handle_to_area(shared_info->dsa);
          dsa_pointer ptr = dsa_allocate(dsa, sizeof(*handle));
          handle = (omni_handle_private *)dsa_get_address(dsa, ptr);
          if (module_info->name != NULL) {
            strncpy(NameStr(handle->module_info_name), module_info->name, NAMEDATALEN - 1);
          } else {
            NameStr(handle->module_info_name)[0] = 0;
          }
          if (module_info->version != NULL) {
            strncpy(NameStr(handle->module_info_version), module_info->version, NAMEDATALEN - 1);
          } else {
            NameStr(handle->module_info_version)[0] = 0;
          }
          if (module_info->identity != NULL) {
            strncpy(NameStr(handle->module_info_identity), module_info->identity, NAMEDATALEN - 1);
          } else {
            NameStr(handle->module_info_identity)[0] = 0;
          }
          handle->magic = *magic;
          pg_atomic_init_u32(&handle->refcount, 0);
          pg_atomic_init_u64(&handle->switchboard, 0);
          strcpy(handle->path, path);
#define set_function_implementations(handle)                                                       \
  handle->register_hook = register_hook;                                                           \
  handle->allocate_shmem =                                                                         \
      magic->revision == 0 ? (omni_allocate_shmem_function)allocate_shmem_0_0 : allocate_shmem;    \
  handle->deallocate_shmem = deallocate_shmem;                                                     \
  handle->lookup_shmem = lookup_shmem;                                                             \
  handle->get_library_name = get_library_name;                                                     \
  handle->declare_guc_variable = declare_guc_variable;                                             \
  handle->request_bgworker_start = request_bgworker_start;                                         \
  handle->request_bgworker_termination = request_bgworker_termination;                             \
  handle->register_lwlock = register_lwlock;                                                       \
  handle->unregister_lwlock = unregister_lwlock;                                                   \
  handle->atomic_switch = atomic_switch
          if (magic->revision < 4) {
            // In 0.4 (0D), we moved `register_hook` below `lookup_shmem`
            struct _omni_handle_0r3 {
              char *(*get_library_name)(const omni_handle *handle);

              omni_allocate_shmem_function allocate_shmem;
              omni_deallocate_shmem_function deallocate_shmem;

              omni_register_hook_function register_hook;

              omni_lookup_shmem_function lookup_shmem;

              omni_declare_guc_variable_function declare_guc_variable;

              omni_request_bgworker_start_function request_bgworker_start;
              omni_request_bgworker_termination_function request_bgworker_termination;

              omni_register_lwlock_function register_lwlock;
              omni_unregister_lwlock_function unregister_lwlock;

              // Not really defined, but here to make `set_function_implementations` work
              omni_atomic_switch_function atomic_switch;
            };

            set_function_implementations(((struct _omni_handle_0r3 *)&handle->handle));
          } else {
            set_function_implementations((&handle->handle));
          }
#undef set_function_implementations
          entry->dsa = handle->dsa = dsa_get_handle(dsa);
          entry->pointer = ptr;
          handle->id = entry->id = id;
        } else {
          dsa_area *entry_dsa = dsa_handle_to_area(entry->dsa);
          handle = dsa_get_address(entry_dsa, entry->pointer);
        }

        result = handle;

        // We no longer need an exclusive lock for modules
        dshash_release_lock(omni_modules, entry);
        LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);

      } else {
        ereport(WARNING, errmsg("Incompatible magic version %d (expected 0)", magic->version));
      }
    } else {
      dlclose(dlhandle);
    }
  }
  return result;
}

static List *consider_ext(HeapTuple tp, TupleDesc pg_extension_tuple_desc) {
  Form_pg_extension ext = (Form_pg_extension)GETSTRUCT(tp);
  List *loaded = NIL;
  bool isnull;
  Datum extver_datum =
      heap_getattr(tp, Anum_pg_extension_extversion, pg_extension_tuple_desc, &isnull);
  if (!isnull) {
    char *extver = text_to_cstring(DatumGetTextPP(extver_datum));
    char *pathname = get_extension_module_pathname(NameStr(ext->extname), extver);
    if (pathname == NULL) {
      return loaded;
    }
    char *path = substitute_libpath_macro(pathname);
    char key[PATH_MAX] = {0};
    strcpy(key, path);
    pfree(path);

    bool warning_on_omni_mismatch = true;
    // If the tuple is added in current transaction, if the version is mismatched, it should
    // be an error, otherwise, a warning.
    //
    // Otherwise, it is impossible to successfully load previously-installed version to proceed
    // further to upgrade to the correct version.
    if (TransactionIdIsValid(GetCurrentTransactionIdIfAny()) &&
        TransactionIdEquals(HeapTupleHeaderGetXmin(tp->t_data), GetCurrentTransactionIdIfAny())) {
      warning_on_omni_mismatch = false;
    }

    omni_handle_private *handle = load_module(key, warning_on_omni_mismatch);
    if (handle != NULL) {
      loaded = list_append_unique_ptr(loaded, handle);
    }
  }
  return loaded;
}

MODULE_FUNCTION void load_pending_modules() {
  if (!ensure_backend_initialized()) {
    return;
  }
  if (IsTransactionState() && backend_force_reload) {
    backend_force_reload = false;
    List *loaded_modules = NIL;

    {
      // Consider pg_extension
      Relation rel = table_open(ExtensionRelationId, RowExclusiveLock);
      TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
      for (;;) {
        HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
        if (tup == NULL)
          break;
        loaded_modules =
            list_concat_unique_ptr(loaded_modules, consider_ext(tup, RelationGetDescr(rel)));
      }
      if (scan->rs_rd->rd_tableam->scan_end) {
        scan->rs_rd->rd_tableam->scan_end(scan);
      }
      table_close(rel, RowExclusiveLock);
    }

    static omni_handle_private *self = NULL;
    {
      // Ensure `omni` itself is loaded (once)
      static bool omni_loaded = false;
      if (unlikely(!omni_loaded)) {
        self = load_module(get_omni_library_name(), false);
        Assert(self != NULL); // must be always true as we're loading the same file
        loaded_modules = list_append_unique_ptr(loaded_modules, self);
        omni_loaded = true;
      }
    }

    // Modules that we need to remove
    List *modules_to_remove = list_difference_ptr(initialized_modules, loaded_modules);
    // Modules that we need to load
    List *modules_to_load = list_difference_ptr(loaded_modules, initialized_modules);

    ListCell *lc;

    // Remove first so that we have a chance to unload on upgrades
    foreach (lc, modules_to_remove) {
      // Unload unless it's `omni` itself
      if (likely((omni_handle_private *)lfirst(lc) != self)) {
        unload_module(lfirst(lc), true);
      }
    }

    // Now we can proceed with loads
    foreach (lc, modules_to_load) {
      omni_handle_private *handle = (omni_handle_private *)lfirst(lc);

      {
        // If the module was not initialized in this backend, do so
        if (!list_member_ptr(initialized_modules, handle)) {

          // Create a memory context
          MemoryContext module_memory_context = AllocSetContextCreate(
              TopMemoryContext, "TopMemoryContext/omni", ALLOCSET_DEFAULT_SIZES);
          // We copy the identifier in the top memory context so that what we reset it
          // upon deinitialization, the identifier is still valid.
          MemoryContextSetIdentifier(module_memory_context,
                                     MemoryContextStrdup(TopMemoryContext, handle->path));

          {
            // Ensure `_Omni_load` and `_Omni_init` are called in module's memory context
            MemoryContext oldcontext = MemoryContextSwitchTo(BackendModuleContextForModule(handle));

            void *dlhandle = dlopen(handle->path, RTLD_LAZY);

            // This backend is the first one to load this module

            if (unlikely(handle->magic.revision < 5)) {
              if (pg_atomic_add_fetch_u32(&handle->refcount, 1) == 1) {
                // Let's also load it if there's a callback
                pg_atomic_init_u32(&handle->loaded, 1);
                void (*load_fn)(const omni_handle *) = dlsym(dlhandle, "_Omni_load");
                if (load_fn != NULL) {
                  load_fn(&handle->handle);
                }
              }
            }

            // Check the target Postgres version
            if (handle->magic.revision >= 7 && handle->magic.pg_version != ServerVersionNum) {
              ereport(WARNING,
                      errmsg("omni has been compiled against %d.%d, but running on %d.%d",
                             handle->magic.pg_version / 10000, handle->magic.pg_version % 100,
                             ServerVersionNum / 10000, ServerVersionNum % 100));
            }

            void (*init_fn)(const omni_handle *) = dlsym(dlhandle, "_Omni_init");
            if (init_fn != NULL) {
              init_fn(&handle->handle);
            }

            MemoryContextSwitchTo(oldcontext);
          }

          MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
          initialized_modules = list_append_unique_ptr(initialized_modules, handle);
          MemoryContextSwitchTo(oldcontext);
        }
      }
    }
  }
}

static void register_hook(const omni_handle *handle, omni_hook *hook) {
  Assert(hook->type >= 0 && hook->type <= __OMNI_HOOK_TYPE_COUNT);
  omni_handle_private *phandle = struct_from_member(omni_handle_private, handle, handle);

  hook_entry_point *entry_point;

  // Ensure we allocate in the top memory context as we need to keep these around forever
  MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);

  if (hook->wrap) {
    int initial_count = hook_entry_points.entry_points_count[hook->type];

    // Increment all index states by one as we're inserting a new element
    // in the front which will assume the first index.
    hook_entry_point *ep = hook_entry_points.entry_points[hook->type];
    for (int i = 0; i < initial_count; i++) {
      ep->state_index++;
      ep++;
    }

    // Increment the count for both elements (before & after)
    hook_entry_points.entry_points_count[hook->type] += 2;

    // Allocate a new array of hooks as we're inject in the front
    entry_point = palloc(sizeof(*hook_entry_points.entry_points[hook->type]) *
                         hook_entry_points.entry_points_count[hook->type]);
    // Copy the old array to the second element of the new one
    memcpy(entry_point + 1, hook_entry_points.entry_points[hook->type],
           sizeof(*hook_entry_points.entry_points[hook->type]) * initial_count);

    // Prepare new first element
    entry_point->handle = handle;
    entry_point->fn = hook->fn;
    entry_point->name = hook->name;
    entry_point->state_index = hook_entry_points.entry_points_count[hook->type] - 1;

    // Free the old array
    pfree(hook_entry_points.entry_points[hook->type]);

    // Ensure we're pointing to the new array
    hook_entry_points.entry_points[hook->type] = entry_point;

  } else {
    // Figure out the size of the array needed while incrementing the counter
    size_t size = sizeof(*hook_entry_points.entry_points[hook->type]) *
                  (++hook_entry_points.entry_points_count[hook->type]);
    if (hook_entry_points.entry_points[hook->type] != NULL) {
      // If there are entrypoints, reallocate
      hook_entry_points.entry_points[hook->type] =
          repalloc(hook_entry_points.entry_points[hook->type], size);
    } else {
      // Otherwise, allocate
      hook_entry_points.entry_points[hook->type] = palloc(size);
    }
  }

  MemoryContextSwitchTo(oldcontext);

  // Add an element to the end of the array
  entry_point = hook_entry_points.entry_points[hook->type] +
                (hook_entry_points.entry_points_count[hook->type] - 1);

  entry_point->handle = handle;
  entry_point->fn = hook->fn;
  entry_point->name = hook->name;
  entry_point->state_index = hook_entry_points.entry_points_count[hook->type] - 1;
}

MODULE_FUNCTION char *get_library_name(const omni_handle *handle) {
  return get_fitting_library_name(struct_from_member(omni_handle_private, handle, handle)->path);
}

static struct dsa_ref {
  dsa_handle handle;
  dsa_pointer pointer;
  ModuleAllocation *alloc;
} find_or_allocate_shmem_dsa(const omni_handle *handle, const char *name, size_t size,
                             omni_allocate_shmem_callback_function init, void *data,
                             HASHACTION action, bool *found) {
  if (strlen(name) > NAMEDATALEN - 1) {
    ereport(ERROR, errmsg("name must be under 64 bytes long"));
  }
  if (size == 0) {
    ereport(ERROR, errmsg("size must be larger than 0"));
  }
  omni_handle_private *phandle = struct_from_member(omni_handle_private, handle, handle);
  LWLockAcquire(&(locks + OMNI_LOCK_ALLOCATION)->lock,
                action == HASH_FIND ? LW_SHARED : LW_EXCLUSIVE);
  ModuleAllocationKey key = {
      .module_id = phandle->id,
  };
  strncpy(key.name, name, sizeof(key.name));
  ModuleAllocation *alloc;
  switch (action) {
  case HASH_ENTER:
  case HASH_ENTER_NULL:
    alloc = (ModuleAllocation *)dshash_find_or_insert(omni_allocations, &key, found);
    break;
  case HASH_FIND:
    alloc = (ModuleAllocation *)dshash_find(omni_allocations, &key, false);
    *found = alloc != NULL;
    break;
  case HASH_REMOVE:
    alloc = (ModuleAllocation *)dshash_find(omni_allocations, &key, true);
    *found = alloc != NULL;
    break;
  }
  if (!*found) {
    if (action == HASH_ENTER || action == HASH_ENTER_NULL) {
      alloc->dsa_handle = shared_info->dsa;
      dsa_area *dsa = dsa_handle_to_area(shared_info->dsa);
      uint32 interrupt_holdoff = InterruptHoldoffCount;
      PG_TRY();
      { alloc->dsa_pointer = dsa_allocate(dsa, size); }
      PG_CATCH();
      {
        // errfinish sets `InterruptHoldoffCount` to zero as part of its cleanup
        // and suggests that handlers can save and restore it itself.
        InterruptHoldoffCount = interrupt_holdoff;
        // If shared memory allocation fails, remove the record
        dshash_delete_entry(omni_allocations, alloc);
        LWLockRelease(&(locks + OMNI_LOCK_ALLOCATION)->lock);
        // and continue with the error
        PG_RE_THROW();
      }
      PG_END_TRY();
      alloc->size = size;
      pg_atomic_init_u32(&alloc->refcounter, 0);

      if (init != NULL) {
        if (phandle->magic.revision < 3) {
          void (*init_rev2)(void *ptr, void *data) = (void (*)(void *ptr, void *data))init;
          init_rev2(dsa_get_address(dsa, alloc->dsa_pointer), data);
        } else {
          init(handle, dsa_get_address(dsa, alloc->dsa_pointer), data, true);
        }
      }
    }
  } else {
    if (init != NULL) {
      if (phandle->magic.revision >= 3) {
        init(handle, dsa_get_address(dsa_handle_to_area(alloc->dsa_handle), alloc->dsa_pointer),
             data, false);
      }
    }
  }
  struct dsa_ref result =
      (*found || (action == HASH_ENTER || action == HASH_ENTER_NULL) // if it was found or allocated
           ? ((struct dsa_ref){
                 .pointer = alloc->dsa_pointer, .handle = alloc->dsa_handle, .alloc = alloc})
           : (struct dsa_ref){});
  if (action == HASH_REMOVE && *found) {
    if (pg_atomic_sub_fetch_u32(&alloc->refcounter, 1) == 0) {
      dshash_delete_entry(omni_allocations, alloc);
      result.alloc = NULL;
    } else {
      dshash_release_lock(omni_allocations, alloc);
    }
  } else if (action == HASH_ENTER || *found) {
    pg_atomic_add_fetch_u32(&alloc->refcounter, 1);
    dshash_release_lock(omni_allocations, alloc);
  }
  LWLockRelease(&(locks + OMNI_LOCK_ALLOCATION)->lock);
  return result;
}

static void *find_or_allocate_shmem(const omni_handle *handle, const char *name, size_t size,
                                    omni_allocate_shmem_callback_function init, void *data,
                                    bool find, bool *found) {
  void *ptr;
  struct dsa_ref ref = find_or_allocate_shmem_dsa(handle, name, size, init, data,
                                                  find ? HASH_FIND : HASH_ENTER, found);
  if (!*found) {
    // If just searching, return NULL, otherwise return allocation
    dsa_area *dsa = dsa_handle_to_area(shared_info->dsa);
    ptr = find ? NULL : dsa_get_address(dsa, ref.pointer);
  } else {
    ptr = dsa_get_address(dsa_handle_to_area(ref.handle), ref.pointer);
  }

  if (ptr != NULL) {
    omni_handle_private *phandle = struct_from_member(omni_handle_private, handle, handle);
    // Allocate the key on module's memory context
    MemoryContext oldcontext = MemoryContextSwitchTo(BackendModuleContextForModule(phandle));
    ModuleAllocationKey *key = palloc(sizeof(*key));
    key->module_id = phandle->id;
    strncpy(key->name, name, sizeof(key->name) - 1);
    // But change the acquisitions list in top memory context
    MemoryContextSwitchTo(TopMemoryContext);
    backend_shmem_acquisitions = list_append_unique_ptr(backend_shmem_acquisitions, key);
    MemoryContextSwitchTo(oldcontext);
  }

  return ptr;
}

static void *allocate_shmem(const omni_handle *handle, const char *name, size_t size,
                            omni_allocate_shmem_callback_function init, void *data, bool *found) {
  return find_or_allocate_shmem(handle, name, size, init, data, false, found);
}

static void *lookup_shmem(const omni_handle *handle, const char *name, bool *found) {
  return find_or_allocate_shmem(handle, name, 1 /* size is ignored in this case */, NULL, NULL,
                                true, found);
}

static void deallocate_shmem(const omni_handle *handle, const char *name, bool *found) {
  struct dsa_ref ref = find_or_allocate_shmem_dsa(
      handle, name, 1 /* size is ignored in this case */, NULL, NULL, HASH_REMOVE, found);
  if (*found) {
    omni_handle_private *phandle = struct_from_member(omni_handle_private, handle, handle);
    MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    ListCell *lc;
    foreach (lc, backend_shmem_acquisitions) {
      ModuleAllocationKey *key = (ModuleAllocationKey *)lfirst(lc);

      if (key->module_id == phandle->id && strcmp(key->name, name) == 0) {
        backend_shmem_acquisitions = foreach_delete_current(backend_shmem_acquisitions, lc);
      }
    }
    MemoryContextSwitchTo(oldcontext);
  }
  if (*found && ref.alloc == NULL) {
    dsa_free(dsa_handle_to_area(ref.handle), ref.pointer);
  }
}

#if PG_MAJORVERSION_NUM < 16
// Extracted from Postgres 15

/*
 * the bare comparison function for GUC names
 */
static int guc_name_compare(const char *namea, const char *nameb) {
  /*
   * The temptation to use strcasecmp() here must be resisted, because the
   * array ordering has to remain stable across setlocale() calls. So, build
   * our own with a simple ASCII-only downcasing.
   */
  while (*namea && *nameb) {
    char cha = *namea++;
    char chb = *nameb++;

    if (cha >= 'A' && cha <= 'Z')
      cha += 'a' - 'A';
    if (chb >= 'A' && chb <= 'Z')
      chb += 'a' - 'A';
    if (cha != chb)
      return cha - chb;
  }
  if (*namea)
    return 1; /* a is longer */
  if (*nameb)
    return -1; /* b is longer */
  return 0;
}
/*
 * comparator for qsorting and bsearching guc_variables array
 */
static int guc_var_compare(const void *a, const void *b) {
  const struct config_generic *confa = *(struct config_generic *const *)a;
  const struct config_generic *confb = *(struct config_generic *const *)b;

  return guc_name_compare(confa->name, confb->name);
}

#endif

static void declare_guc_variable(const omni_handle *handle, omni_guc_variable *variable) {
  Assert(variable);
#if PG_MAJORVERSION_NUM >= 16
  struct config_generic *config = find_option(variable->name, false, true, WARNING);
#else
  const char **key = &(variable->name);
  struct config_generic **configp = (struct config_generic **)bsearch(
      (void *)&key, (void *)get_guc_variables(), GetNumConfigOptions(),
      sizeof(struct config_generic *), guc_var_compare);
  struct config_generic *config = configp ? *configp : NULL;
#endif
  if (config == NULL || (config->flags & GUC_CUSTOM_PLACEHOLDER) != 0) {
    switch (variable->type) {
    case PGC_BOOL:
      // Zero-allocating to ensure the value is valid for GUC
      variable->typed.bool_val.value =
          MemoryContextAllocExtended(OmniGUCContext, sizeof(bool), MCXT_ALLOC_ZERO);
      DefineCustomBoolVariable(variable->name, variable->short_desc, variable->long_desc,
                               variable->typed.bool_val.value, variable->typed.bool_val.boot_value,
                               variable->context, variable->flags,
                               variable->typed.bool_val.check_hook,
                               variable->typed.bool_val.assign_hook, variable->show_hook);
      break;
    case PGC_INT:
      // Zero-allocating to ensure the value is valid for GUC
      variable->typed.int_val.value =
          MemoryContextAllocExtended(OmniGUCContext, sizeof(int), MCXT_ALLOC_ZERO);
      DefineCustomIntVariable(variable->name, variable->short_desc, variable->long_desc,
                              variable->typed.int_val.value, variable->typed.int_val.boot_value,
                              variable->typed.int_val.min_value, variable->typed.int_val.max_value,
                              variable->context, variable->flags,
                              variable->typed.int_val.check_hook,
                              variable->typed.int_val.assign_hook, variable->show_hook);
      break;
    case PGC_REAL:
      // Zero-allocating to ensure the value is valid for GUC
      variable->typed.real_val.value =
          MemoryContextAllocExtended(OmniGUCContext, sizeof(double), MCXT_ALLOC_ZERO);
      DefineCustomRealVariable(variable->name, variable->short_desc, variable->long_desc,
                               variable->typed.real_val.value, variable->typed.real_val.boot_value,
                               variable->typed.real_val.min_value,
                               variable->typed.real_val.max_value, variable->context,
                               variable->flags, variable->typed.real_val.check_hook,
                               variable->typed.real_val.assign_hook, variable->show_hook);
      break;
    case PGC_STRING:
      // Zero-allocating to ensure the value is valid for GUC
      variable->typed.string_val.value =
          MemoryContextAllocExtended(OmniGUCContext, sizeof(char *), MCXT_ALLOC_ZERO);
      DefineCustomStringVariable(variable->name, variable->short_desc, variable->long_desc,
                                 variable->typed.string_val.value,
                                 variable->typed.string_val.boot_value, variable->context,
                                 variable->flags, variable->typed.string_val.check_hook,
                                 variable->typed.string_val.assign_hook, variable->show_hook);
      break;
    case PGC_ENUM:
      variable->typed.enum_val.value = MemoryContextAlloc(OmniGUCContext, sizeof(int));
      *variable->typed.enum_val.value = variable->typed.enum_val.boot_value;
      DefineCustomEnumVariable(variable->name, variable->short_desc, variable->long_desc,
                               variable->typed.enum_val.value, variable->typed.enum_val.boot_value,
                               variable->typed.enum_val.options, variable->context, variable->flags,
                               variable->typed.enum_val.check_hook,
                               variable->typed.enum_val.assign_hook, variable->show_hook);
      break;
    default:
      ereport(ERROR, errmsg("not supported"));
    }
  } else {
    if (config->vartype != variable->type) {
      ereport(ERROR, errmsg("mismatched variable type for %s", variable->name));
    }
    switch (variable->type) {
    case PGC_BOOL:
      variable->typed.bool_val.value = ((struct config_bool *)config)->variable;
    case PGC_INT:
      variable->typed.int_val.value = ((struct config_int *)config)->variable;
      break;
    case PGC_REAL:
      variable->typed.real_val.value = ((struct config_real *)config)->variable;
      break;
    case PGC_STRING:
      variable->typed.string_val.value = ((struct config_string *)config)->variable;
      break;
    case PGC_ENUM:
      variable->typed.enum_val.value = ((struct config_enum *)config)->variable;
      break;
    default:
      ereport(ERROR, errmsg("not supported"));
    }
  }
}

struct omni_bgworker_handle_private {
  omni_bgworker_handle handle;
  pid_t pid;
};

struct bgworker_request_payload {
  BackgroundWorker bgworker;
  omni_bgworker_options options;
  struct omni_bgworker_handle *handle;
};

static void do_start_bgworker(XactEvent event, void *arg) {
  struct bgworker_request_payload *payload = (struct bgworker_request_payload *)arg;
  if (event == XACT_EVENT_COMMIT) {
    BackgroundWorkerHandle *bgw_handle;
    RegisterDynamicBackgroundWorker(&payload->bgworker, &bgw_handle);
    payload->handle->registered = true;
    if (!payload->options.dont_wait) {
      pid_t pid;
      WaitForBackgroundWorkerStartup(bgw_handle, &pid);
    }
    payload->handle->bgw_handle = *bgw_handle;
  }
}

static void request_bgworker_op(const omni_handle *handle, BackgroundWorker *bgworker,
                                omni_bgworker_handle *bgw_handle,
                                const omni_bgworker_options options, XactCallback op) {
  struct bgworker_request_payload *payload = MemoryContextAllocExtended(
      options.timing == omni_timing_immediately ? CurrentMemoryContext : TopTransactionContext,
      sizeof(*payload), MCXT_ALLOC_ZERO);
  if (bgworker != NULL) {
    memcpy(&payload->bgworker, bgworker, sizeof(*bgworker));
  }
  payload->options = options;
  payload->handle = bgw_handle;
  List **oneshot_list;
  switch (options.timing) {
  case omni_timing_immediately:
    op(XACT_EVENT_COMMIT /* pretend */, payload);
    return;
  case omni_timing_at_commit:
    oneshot_list = &xact_oneshot_callbacks;
    break;
  case omni_timing_after_commit:
    oneshot_list = &after_xact_oneshot_callbacks;
    break;
  }

  MemoryContext oldcontext = MemoryContextSwitchTo(TopTransactionContext);
  struct xact_oneshot_callback *cb = palloc(sizeof(*cb));
  cb->fn = op;
  cb->arg = payload;
  *oneshot_list = list_append_unique_ptr(*oneshot_list, cb);
  MemoryContextSwitchTo(oldcontext);
}

static void request_bgworker_start(const omni_handle *handle, BackgroundWorker *bgworker,
                                   omni_bgworker_handle *bgw_handle,
                                   const omni_bgworker_options options) {
  request_bgworker_op(handle, bgworker, bgw_handle, options, do_start_bgworker);
}

static void do_stop_bgworker(XactEvent event, void *arg) {
  struct bgworker_request_payload *payload = (struct bgworker_request_payload *)arg;
  if (event == XACT_EVENT_COMMIT) {
    TerminateBackgroundWorker(&payload->handle->bgw_handle);
    if (!payload->options.dont_wait) {
      // This is similar to `WaitForBackgroundWorkerShutdown` but we can't fully rely on
      // being the PID set for notification in bgw_notify_pid
      BgwHandleStatus status;
      int rc;

      for (;;) {
        pid_t pid;

        CHECK_FOR_INTERRUPTS();

        status = GetBackgroundWorkerPid(&payload->handle->bgw_handle, &pid);
        if (status == BGWH_STOPPED)
          break;

        // Particularly, we have to use timeout here
        rc = WaitLatch(MyLatch, WL_LATCH_SET | WL_POSTMASTER_DEATH | WL_TIMEOUT,
                       50 /* arbitrary timeout of 50ms*/, WAIT_EVENT_BGWORKER_SHUTDOWN);

        if (rc & WL_POSTMASTER_DEATH) {
          break;
        }

        ResetLatch(MyLatch);
      }
    }
  }
}

void request_bgworker_termination(const omni_handle *handle, omni_bgworker_handle *bgw_handle,
                                  const omni_bgworker_options options) {
  request_bgworker_op(handle, NULL, bgw_handle, options, do_stop_bgworker);
}

MODULE_FUNCTION void unload_module(omni_handle_private *phandle, bool missing_ok) {
  LWLockAcquire(&(locks + OMNI_LOCK_MODULE)->lock, LW_EXCLUSIVE);
  bool found = false;
  ModuleEntry *module = (ModuleEntry *)dshash_find_or_insert(omni_modules, phandle->path, &found);
  if (!found) {
    LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);
    if (!missing_ok) {
      ereport(ERROR, errmsg("module id %u not found", phandle->id));
    } else {
      return;
    }
  }

  MemoryContext module_memory_context = BackendModuleContextForModule(phandle);
  if (module_memory_context == NULL) {
    LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);
    ereport(ERROR, errcode(ERRCODE_INTERNAL_ERROR),
            errmsg("no module memory context found for %s", phandle->path));
  }

  // Decrement the refcount
  pg_atomic_add_fetch_u32(&phandle->refcount, -1);

  // Exclude the module from this backend
  MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
  initialized_modules = list_delete_ptr(initialized_modules, phandle);
  MemoryContextSwitchTo(oldcontext);
  // Purge hooks
  reorganize_hooks();

  // Deinitialize this backend
  void *dlhandle = dlopen(phandle->path, RTLD_LAZY);
  {
    MemoryContext oldcontext = MemoryContextSwitchTo(module_memory_context);
    void (*deinit)(const omni_handle *) = dlsym(dlhandle, "_Omni_deinit");
    if (deinit != NULL) {
      deinit(&phandle->handle);
    }
    MemoryContextSwitchTo(oldcontext);
  }

  if (unlikely(phandle->magic.revision < 5)) {
    // Try to unload the module
    uint32 loaded = 1;
    // If it was found and if we were able to capture `loaded` set to `1`
    if (found && pg_atomic_compare_exchange_u32(&phandle->loaded, &loaded, 0)) {
      // Perform unloading
      MemoryContext oldcontext = MemoryContextSwitchTo(module_memory_context);
      void (*unload)(const omni_handle *) = dlsym(dlhandle, "_Omni_unload");
      if (unload != NULL) {
        unload(&phandle->handle);
      }
      MemoryContextSwitchTo(oldcontext);
      dlclose(dlhandle);
    }
  }
  {
    // Cleanup

    // Because we're going to reset the module's memory context below, we need to make sure
    // that shmem acquisitions recorded for this module need to be removed from the list to
    // keep all pointers in that list valid.
    ListCell *lc;
    MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    foreach (lc, backend_shmem_acquisitions) {
      ModuleAllocationKey *key = (ModuleAllocationKey *)lfirst(lc);
      if (key->module_id == phandle->id) {
        backend_shmem_acquisitions = foreach_delete_current(backend_shmem_acquisitions, lc);
      }
    }
    MemoryContextSwitchTo(oldcontext);

    // Instead of deleting the memory context, just release all the space and
    // children, in case we'll use this module record again
    MemoryContextReset(module_memory_context);
  }
  // We no longer need the lock
  dshash_release_lock(omni_modules, module);
  LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);
}

// When backend is exiting, we need to clean up some resources
void deinitialize_backend(int code, Datum arg) {

  if (code == 1) {
    // If exiting because of a `FATAL` error, don't try to deinitialize.
    return;
  }

  // If we didn't have a chance to initialize, bail
  if (!backend_initialized) {
    return;
  }

  bool in_xact = IsTransactionState();

  // Start transaction if are not in one
  if (!in_xact) {
    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    PushActiveSnapshot(GetTransactionSnapshot());
  }

  // Ensure we deinitialize modules that are supposed to be deinitialized
  load_pending_modules();

  // Abort the transaction if we had to start one
  if (!in_xact) {
    PopActiveSnapshot();
    AbortCurrentTransaction();
  }

  // If no allocations were ever made, bail
  if (omni_allocations == NULL) {
    return;
  }

  // Out of abundance of caution, lock
  LWLockAcquire(&(locks + OMNI_LOCK_ALLOCATION)->lock, LW_EXCLUSIVE);

  ListCell *lc;
  // For every module that is still initialized
  foreach (lc, initialized_modules) {
    omni_handle_private *handle = (omni_handle_private *)lfirst(lc);
    MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    ListCell *lc1;
    // Scan through all backend shmem acquisitions
    foreach (lc1, backend_shmem_acquisitions) {
      ModuleAllocationKey *key = (ModuleAllocationKey *)lfirst(lc1);
      // If the module ID is matching, means we acquired this shmem
      if (key->module_id == handle->id) {
        // Decrement the refcounter
        ModuleAllocation *allocation = dshash_find(omni_allocations, key, true);
        if (allocation != NULL) {
          pg_atomic_sub_fetch_u32(&allocation->refcounter, 1);
          dshash_release_lock(omni_allocations, allocation);
        }
      }
    }
    MemoryContextSwitchTo(oldcontext);
  }
  backend_shmem_acquisitions = NIL;

  LWLockRelease(&(locks + OMNI_LOCK_ALLOCATION)->lock);
}
