// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/heapam.h>
#include <access/table.h>
#include <catalog/pg_language.h>
#include <catalog/pg_proc.h>
#include <commands/user.h>
#include <executor/executor.h>
#include <miscadmin.h>
#include <storage/ipc.h>
#include <storage/latch.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/builtins.h>
#include <utils/hsearch.h>
#include <utils/memutils.h>
#include <utils/rel.h>
#include <utils/snapmgr.h>
#include <utils/syscache.h>
#if PG_MAJORVERSION_NUM >= 14
#include <utils/wait_event.h>
#else
#include <pgstat.h>
#endif

#include "omni_common.h"

PG_MODULE_MAGIC;
OMNI_MAGIC;

omni_shared_info *shared_info;

MODULE_VARIABLE(dshash_table *omni_modules);
MODULE_VARIABLE(dshash_table *omni_allocations);
MODULE_VARIABLE(HTAB *dsa_handles);

MODULE_VARIABLE(omni_handle_private *module_handles);

MODULE_VARIABLE(LWLockPadded *locks);

static TupleDesc pg_proc_tuple_desc;
MODULE_VARIABLE(List *initialized_modules);

MODULE_VARIABLE(int OMNI_DSA_TRANCHE);

MODULE_VARIABLE(bool backend_force_reload);

MODULE_VARIABLE(MemoryContext OmniGUCContext);

static dsa_area *dsa = NULL;

MODULE_FUNCTION dsa_area *dsa_handle_to_area(dsa_handle handle) {
  if (handle != dsa_get_handle(dsa)) {
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
  const dshash_parameters module_params = {.key_size = PATH_MAX,
                                           .entry_size = sizeof(ModuleEntry),
                                           .hash_function = dshash_memhash,
                                           .compare_function = dshash_memcmp,
                                           .tranche_id = OMNI_DSA_TRANCHE};
  const dshash_parameters allocation_params = {.key_size = sizeof(ModuleAllocationKey),
                                               .entry_size = sizeof(ModuleAllocation),
                                               .hash_function = dshash_memhash,
                                               .compare_function = dshash_memcmp,
                                               .tranche_id = OMNI_DSA_TRANCHE};
  MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
  if (pg_atomic_test_set_flag(&shared_info->tables_initialized)) {
    // Nobody has initialized modules table yet. And we're under an exclusive lock,
    // so we can do it
    omni_modules = dshash_create(dsa, &module_params, NULL);
    shared_info->modules_tab = dshash_get_hash_table_handle(omni_modules);
    omni_allocations = dshash_create(dsa, &allocation_params, NULL);
    shared_info->allocations_tab = dshash_get_hash_table_handle(omni_allocations);
    shared_info->dsa = dsa_get_handle(dsa);
  } else {
    // Otherwise, attach to it
    dsa_area *dsa_area = dsa_handle_to_area(shared_info->dsa);
    omni_modules = dshash_attach(dsa_area, &module_params, shared_info->modules_tab, NULL);
    omni_allocations =
        dshash_attach(dsa_area, &allocation_params, shared_info->allocations_tab, NULL);
  }
  MemoryContextSwitchTo(oldcontext);
}

static void ensure_backend_initialized(void) {
  // Indicates whether we have ever done anything on this backend
  static bool backend_initialized = false;
  if (backend_initialized)
    return;

  // If we're out of any transaction (for example, `ShutdownPostgres` would do that), bail
  if (!IsTransactionState()) {
    return;
  }

  LWLockRegisterTranche(OMNI_DSA_TRANCHE, "omni:dsa");

  {
    // It is important to allocate DSA in the top memory context
    // so it doesn't get deallocated when the context we're in is gone
    MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    dsa = dsa_create(OMNI_DSA_TRANCHE);
    MemoryContextSwitchTo(oldcontext);
    dsa_pin(dsa);
    dsa_pin_mapping(dsa);
  }

  {
    // Get `pg_proc` TupleDesc
    Relation rel = table_open(ProcedureRelationId, AccessShareLock);
    pg_proc_tuple_desc = RelationGetDescr(rel);
    table_close(rel, AccessShareLock);
  }

  locks = GetNamedLWLockTranche("omni");

  {
    HASHCTL ctl = {.keysize = sizeof(dsa_handle), .entrysize = sizeof(DSAHandleEntry)};
    dsa_handles = hash_create("omni:dsa_handles", 128, &ctl, HASH_ELEM | HASH_BLOBS);
  }

  initialize_omni_modules();

  backend_initialized = true;
  initialized_modules = NIL;
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
  system(psprintf("syslog -s -k Facility com.apple.console -k Level notice -k Message 'omnigres "
                  "register %d %p %d %s'",
                  initialize, lock, tranche_id, name));

  if (initialize) {
    LWLockInitialize(lock, tranche_id);
  }
}

#if PG_MAJORVERSION_NUM >= 13 && PG_MAJORVERSION_NUM <= 16
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

static List *consider_probin(HeapTuple tp) {
  Form_pg_proc proc = (Form_pg_proc)GETSTRUCT(tp);
  List *loaded = NIL;
  if (proc->prolang == ClanguageId) {
    // If it is a module implemented in C, we can start looking into whether this is an Omni module
    bool isnull;
    Datum probin = heap_getattr(tp, Anum_pg_proc_probin, pg_proc_tuple_desc, &isnull);
    if (!isnull) {
      char *path = substitute_libpath_macro(text_to_cstring(DatumGetTextPP(probin)));
      char key[PATH_MAX] = {0};
      strcpy(key, path);
      pfree(path);

      // Try to load it
      void *dlhandle = dlopen(key, RTLD_LAZY);

      if (dlhandle != NULL) {
        // If loaded, check for magic header
        omni_magic *(*magic_fn)() = dlsym(dlhandle, "_Omni_magic");
        if (magic_fn != NULL) {
          // Check if magic is correct
          omni_magic *magic = magic_fn();
          if (magic->size == sizeof(omni_magic) && magic->version == OMNI_INTERFACE_VERSION) {
            // We are going to record it if it wasn't yet
            LWLockAcquire(&(locks + OMNI_LOCK_MODULE)->lock, LW_EXCLUSIVE);
            bool found = false;
            ModuleEntry *entry = dshash_find_or_insert(omni_modules, key, &found);
            omni_handle_private *handle;

            if (!found) {
              uint32 id = pg_atomic_add_fetch_u32(&shared_info->module_counter, 1);

              // If not found, prepare the handle
              dsa_pointer ptr = dsa_allocate(dsa, sizeof(*handle));
              handle = (omni_handle_private *)dsa_get_address(dsa, ptr);
              handle->magic = *magic;
              pg_atomic_init_u32(&handle->refcount, 0);
              strcpy(handle->path, key);
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
  handle->unregister_lwlock = unregister_lwlock
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
                };

                set_function_implementations(((struct _omni_handle_0r3 *)&handle->handle));
              } else {
                set_function_implementations((&handle->handle));
              }
              entry->dsa = handle->dsa = dsa_get_handle(dsa);
              entry->pointer = ptr;
              handle->id = entry->id = id;
            } else {
              dsa_area *entry_dsa = dsa_handle_to_area(entry->dsa);
              handle = dsa_get_address(entry_dsa, entry->pointer);
            }

            loaded = list_append_unique_ptr(loaded, handle);

            // We no longer need an exclusive lock for modules
            dshash_release_lock(omni_modules, entry);
            LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);

            {
              // If the module was not initialized in this backend, do so
              if (!list_member_ptr(initialized_modules, handle)) {

                // Create a memory context
                MemoryContext module_memory_context = AllocSetContextCreate(
                    TopMemoryContext, "TopMemoryContext/omni", ALLOCSET_DEFAULT_SIZES);
                // We copy the identifier in the top memory context so that what we reset it
                // upon deinitialization, the identifier is still valid.
                MemoryContextSetIdentifier(module_memory_context,
                                           MemoryContextStrdup(TopMemoryContext, key));

                {
                  // Ensure `_Omni_load` and `_Omni_init` are called in module's memory context
                  MemoryContext oldcontext = MemoryContextSwitchTo(module_memory_context);

                  // This backend is the first one to load this module
                  if (pg_atomic_add_fetch_u32(&handle->refcount, 1) == 1) {
                    // Let's also load it if there's a callback
                    pg_atomic_init_u32(&handle->loaded, 1);
                    void (*load_fn)(const omni_handle *) = dlsym(dlhandle, "_Omni_load");
                    if (load_fn != NULL) {
                      load_fn(&handle->handle);
                    }
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

          } else {
            ereport(WARNING, errmsg("Incompatible magic version %d (expected 0)", magic->version));
          }
        } else {
          dlclose(dlhandle);
        }
      }
    }
  }
  return loaded;
}

MODULE_FUNCTION void load_pending_modules() {
  ensure_backend_initialized();

  if (IsTransactionState() && backend_force_reload) {
    backend_force_reload = false;
    Relation rel = table_open(ProcedureRelationId, RowExclusiveLock);
    TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
    List *loaded_modules = NIL;
    for (;;) {
      HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
      if (tup == NULL)
        break;
      loaded_modules = list_concat_unique_ptr(loaded_modules, consider_probin(tup));
    }
    if (scan->rs_rd->rd_tableam->scan_end) {
      scan->rs_rd->rd_tableam->scan_end(scan);
    }
    table_close(rel, RowExclusiveLock);

    List *removed_modules = list_difference_ptr(initialized_modules, loaded_modules);
    ListCell *lc;
    foreach (lc, removed_modules) {
      unload_module(lfirst(lc), true);
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

    // Shift index states
    for (int i = 0; i < initial_count; i++) {
      hook_entry_points.entry_points[hook->type]->state_index = i + 1;
    }

    hook_entry_points.entry_points_count[hook->type] += 2;
    entry_point = palloc(sizeof(*hook_entry_points.entry_points[hook->type]) *
                         hook_entry_points.entry_points_count[hook->type]);
    memcpy(entry_point + 1, hook_entry_points.entry_points[hook->type],
           sizeof(*hook_entry_points.entry_points[hook->type]) * initial_count);

    entry_point->handle = handle;
    entry_point->fn = hook->fn.ptr;
    entry_point->name = hook->name;
    entry_point->state_index = hook_entry_points.entry_points_count[hook->type] - 1;

    hook_entry_points.entry_points[hook->type] = entry_point;

  } else {
    hook_entry_points.entry_points[hook->type] =
        repalloc(hook_entry_points.entry_points[hook->type],
                 sizeof(*hook_entry_points.entry_points[hook->type]) *
                     (++hook_entry_points.entry_points_count[hook->type]));
  }

  MemoryContextSwitchTo(oldcontext);

  entry_point = hook_entry_points.entry_points[hook->type] +
                (hook_entry_points.entry_points_count[hook->type] - 1);

  entry_point->handle = handle;
  entry_point->fn = hook->fn.ptr;
  entry_point->name = hook->name;
  entry_point->state_index = hook_entry_points.entry_points_count[hook->type] - 1;
}

MODULE_FUNCTION char *get_library_name(const omni_handle *handle) {
  return get_fitting_library_name(struct_from_member(omni_handle_private, handle, handle)->path);
}

static struct dsa_ref {
  dsa_handle handle;
  dsa_pointer pointer;
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
      alloc->dsa_handle = dsa_get_handle(dsa);
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

      if (init != NULL) {
        if (phandle->magic.revision < 3) {
          void (*init_rev2)(void *ptr, void *data) = (void (*)(void *ptr, void *data))init;
          init_rev2(dsa_get_address(dsa, alloc->dsa_pointer), data);
        } else {
          init(handle, dsa_get_address(dsa, alloc->dsa_pointer), data, true);
        }
      }
    }
  } else if (init != NULL) {
    if (phandle->magic.revision >= 3) {
      init(handle, dsa_get_address(dsa_handle_to_area(alloc->dsa_handle), alloc->dsa_pointer), data,
           false);
    }
  }
  struct dsa_ref result =
      (*found || (action == HASH_ENTER || action == HASH_ENTER_NULL) // if it was found or allocated
           ? ((struct dsa_ref){.pointer = alloc->dsa_pointer, .handle = alloc->dsa_handle})
           : (struct dsa_ref){});
  if (action == HASH_REMOVE && *found) {
    dshash_delete_entry(omni_allocations, alloc);
  } else if (action == HASH_ENTER || *found) {
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
    ptr = find ? NULL : dsa_get_address(dsa, ref.pointer);
  } else {
    ptr = dsa_get_address(dsa_handle_to_area(ref.handle), ref.pointer);
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

#if PG_MAJORVERSION_NUM < 16
static void xact_request_bgworker_start(XactEvent event, void *arg);
static void unregister_xact_request_bgworker_start(void *arg) {
  UnregisterXactCallback(xact_request_bgworker_start, arg);
}
#endif

struct bgworker_request_payload {
  BackgroundWorker bgworker;
  omni_bgworker_options options;
  struct omni_bgworker_handle *handle;
};

static void do_start_bgworker(void *arg) {
  struct bgworker_request_payload *payload = (struct bgworker_request_payload *)arg;
  BackgroundWorkerHandle *bgw_handle;
  RegisterDynamicBackgroundWorker(&payload->bgworker, &bgw_handle);
  payload->handle->registered = true;
  if (!payload->options.dont_wait) {
    pid_t pid;
    WaitForBackgroundWorkerStartup(bgw_handle, &pid);
  }
  payload->handle->bgw_handle = *bgw_handle;
}

static void xact_request_bgworker_start(XactEvent event, void *arg) {
  struct bgworker_request_payload *payload = (struct bgworker_request_payload *)arg;
  if (event == XACT_EVENT_COMMIT) {
    if (payload->options.timing == omni_timing_at_commit) {
      do_start_bgworker(arg);
    } else if (payload->options.timing == omni_timing_after_commit) {
      MemoryContextCallback *cb = MemoryContextAlloc(TopTransactionContext, sizeof(*cb));
      cb->func = do_start_bgworker;
      cb->arg = arg;
      MemoryContextRegisterResetCallback(TopTransactionContext, cb);
    }

    // Unregister this callback
#if PG_MAJORVERSION_NUM >= 16
    UnregisterXactCallback(xact_request_bgworker_start, arg);
#else
    {
      MemoryContextCallback *cb = MemoryContextAlloc(TopTransactionContext, sizeof(*cb));
      cb->func = unregister_xact_request_bgworker_start;
      cb->arg = arg;
      MemoryContextRegisterResetCallback(TopTransactionContext, cb);
    }
#endif
  }
}

static void request_bgworker_start(const omni_handle *handle, BackgroundWorker *bgworker,
                                   omni_bgworker_handle *bgw_handle,
                                   const omni_bgworker_options options) {
  struct bgworker_request_payload *payload =
      MemoryContextAlloc(TopTransactionContext, sizeof(*payload));
  memcpy(&payload->bgworker, bgworker, sizeof(*bgworker));
  payload->options = options;
  payload->handle = bgw_handle;
  if (options.timing == omni_timing_immediately) {
    do_start_bgworker(payload);
  } else {
    RegisterXactCallback(xact_request_bgworker_start, payload);
  }
}

static void do_stop_bgworker(void *arg) {
  struct bgworker_request_payload *payload = (struct bgworker_request_payload *)arg;
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

#if PG_MAJORVERSION_NUM < 16
static void xact_request_bgworker_stop(XactEvent event, void *arg);
static void unregister_xact_request_bgworker_stop(void *arg) {
  UnregisterXactCallback(xact_request_bgworker_stop, arg);
}
#endif

static void xact_request_bgworker_stop(XactEvent event, void *arg) {
  struct bgworker_request_payload *payload = (struct bgworker_request_payload *)arg;
  if (event == XACT_EVENT_COMMIT) {
    if (payload->options.timing == omni_timing_at_commit) {
      do_stop_bgworker(arg);
    } else if (payload->options.timing == omni_timing_after_commit) {
      MemoryContextCallback *cb = MemoryContextAlloc(TopTransactionContext, sizeof(*cb));
      cb->func = do_stop_bgworker;
      cb->arg = arg;
      MemoryContextRegisterResetCallback(TopTransactionContext, cb);
    }

    // Unregister this callback
#if PG_MAJORVERSION_NUM >= 16
    UnregisterXactCallback(xact_request_bgworker_stop, arg);
#else
    {
      MemoryContextCallback *cb = MemoryContextAlloc(TopTransactionContext, sizeof(*cb));
      cb->func = unregister_xact_request_bgworker_stop;
      cb->arg = arg;
      MemoryContextRegisterResetCallback(TopTransactionContext, cb);
    }
#endif
  }
}

void request_bgworker_termination(const omni_handle *handle, omni_bgworker_handle *bgworker_handle,
                                  const omni_bgworker_options options) {
  struct bgworker_request_payload *payload = MemoryContextAllocExtended(
      options.timing == omni_timing_immediately ? CurrentMemoryContext : TopTransactionContext,
      sizeof(*payload), MCXT_ALLOC_ZERO);
  payload->handle = bgworker_handle;
  payload->options = options;
  if (options.timing == omni_timing_immediately) {
    do_stop_bgworker(payload);
  } else {
    RegisterXactCallback(xact_request_bgworker_stop, payload);
  }
}

static MemoryContext BackendModuleContextForModule(omni_handle_private *phandle) {
  for (MemoryContext cxt = TopMemoryContext->firstchild; cxt != NULL; cxt = cxt->nextchild) {
    if (strcmp(cxt->name, "TopMemoryContext/omni") == 0 && strcmp(cxt->ident, phandle->path) == 0) {
      return cxt;
    }
  }
  return NULL;
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
  // Exclude the module from this backend
  MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
  initialized_modules = list_delete_ptr(initialized_modules, phandle);
  MemoryContextSwitchTo(oldcontext);
  // Purge hooks
  reorganize_hooks();

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
  // Insted of deleting the memory context, just release all the space and
  // children, in case we'll use this module record again
  MemoryContextReset(module_memory_context);
  // We no longer need the lock
  dshash_release_lock(omni_modules, module);
  LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);
}