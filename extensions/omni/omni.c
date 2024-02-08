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
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/builtins.h>
#include <utils/hsearch.h>
#include <utils/memutils.h>
#include <utils/rel.h>
#include <utils/snapmgr.h>
#include <utils/syscache.h>

#include "omni_common.h"

PG_MODULE_MAGIC;
OMNI_MAGIC;

omni_shared_info *shared_info;

MODULE_VARIABLE(HTAB *omni_modules);
MODULE_VARIABLE(HTAB *omni_allocations);
MODULE_VARIABLE(HTAB *dsa_handles);

MODULE_VARIABLE(omni_handle_private *module_handles);

MODULE_VARIABLE(LWLockPadded *locks);

static TupleDesc pg_proc_tuple_desc;
MODULE_VARIABLE(List *initialized_modules);

MODULE_VARIABLE(int OMNI_DSA_TRANCHE);

MODULE_VARIABLE(bool backend_force_reload);

MODULE_VARIABLE(MemoryContext OmniGUCContext);

MODULE_FUNCTION void ensure_backend_initialized(void) {
  // Indicates whether we have ever done anything on this backend
  static bool backend_initialized = false;
  if (backend_initialized)
    return;

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

  backend_initialized = true;
  initialized_modules = NIL;
}

MODULE_FUNCTION void register_hook(const omni_handle *handle, omni_hook *hook);
MODULE_FUNCTION void *allocate_shmem(const omni_handle *handle, const char *name, size_t size,
                                     void (*init)(void *ptr, void *data), void *data, bool *found);
static void *allocate_shmem_0_0(const omni_handle *handle, const char *name, size_t size,
                                bool *found) {
  return allocate_shmem(handle, name, size, NULL, NULL, found);
}
MODULE_FUNCTION void deallocate_shmem(const omni_handle *handle, const char *name, bool *found);
MODULE_FUNCTION void *lookup_shmem(const omni_handle *handle, const char *name, bool *found);
static void declare_guc_variable(const omni_handle *handle, omni_guc_variable *variable);

/*
 * Substitute for any macros appearing in the given string.
 * Result is always freshly palloc'd.
 */
MODULE_FUNCTION char *substitute_libpath_macro(const char *name) {
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

MODULE_FUNCTION List *consider_probin(HeapTuple tp) {
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
            ModuleEntry *entry = hash_search(omni_modules, key, HASH_ENTER, &found);
            omni_handle_private *handle;

            if (!found) {
              uint32 id = pg_atomic_add_fetch_u32(&shared_info->module_counter, 1);
              handle = module_handles + id;
              // If not found, prepare the handle
              handle->id = id;
              handle->magic = *magic;
              pg_atomic_init_u32(&handle->refcount, 0);
              strcpy(handle->path, key);
              handle->handle.register_hook = register_hook;
              handle->handle.allocate_shmem = magic->revision == 0
                                                  ? (omni_allocate_shmem_function)allocate_shmem_0_0
                                                  : allocate_shmem; // 0A -> 0B
              handle->handle.deallocate_shmem = deallocate_shmem;
              handle->handle.lookup_shmem = lookup_shmem;
              handle->handle.get_library_name = get_library_name;
              handle->handle.declare_guc_variable = declare_guc_variable;
              entry->id = id;
              handle->dsa = 0;
            } else {
              handle = module_handles + entry->id;
            }

            loaded = list_append_unique_int(loaded, entry->id);

            // We no longer need an exclusive lock for modules
            LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);

            {
              // If the module was not initialized in this backend, do so
              if (!list_member_int(initialized_modules, entry->id)) {

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
                MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
                initialized_modules = list_append_unique_int(initialized_modules, entry->id);
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
  static bool first_time = true;

  if (IsTransactionState() && backend_force_reload) {
    backend_force_reload = false;
    Relation rel = table_open(ProcedureRelationId, RowExclusiveLock);
    TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
    List *loaded_modules = NIL;
    for (;;) {
      HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
      if (tup == NULL)
        break;
      loaded_modules = list_concat_unique_int(loaded_modules, consider_probin(tup));
    }
    if (scan->rs_rd->rd_tableam->scan_end) {
      scan->rs_rd->rd_tableam->scan_end(scan);
    }
    table_close(rel, RowExclusiveLock);
    first_time = false;

    List *removed_modules = list_difference_int(initialized_modules, loaded_modules);
    ListCell *lc;
    foreach (lc, removed_modules) {
      unload_module(lfirst_int(lc), true);
    }
  }
}

MODULE_FUNCTION void register_hook(const omni_handle *handle, omni_hook *hook) {
  Assert(hook->type >= 0 && hook->type <= __OMNI_HOOK_TYPE_COUNT);

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
    entry_point->name = hook->name ? pstrdup(hook->name) : NULL;
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
  entry_point->name = hook->name ? pstrdup(hook->name) : NULL;
  entry_point->state_index = hook_entry_points.entry_points_count[hook->type] - 1;
}

MODULE_FUNCTION char *get_library_name(const omni_handle *handle) {
  return get_fitting_library_name(struct_from_member(omni_handle_private, handle, handle)->path);
}

static dsa_area *dsa = NULL;

static struct dsa_ref {
  dsa_handle handle;
  dsa_pointer pointer;
} find_or_allocate_shmem_dsa(const omni_handle *handle, const char *name, size_t size,
                             void (*init)(void *ptr, void *data), void *data, HASHACTION action,
                             bool *found) {
  if (strlen(name) > NAMEDATALEN - 1) {
    ereport(ERROR, errmsg("name must be under 64 bytes long"));
  }
  if (size == 0) {
    ereport(ERROR, errmsg("size must be larger than 0"));
  }
  omni_handle_private *phandle = struct_from_member(omni_handle_private, handle, handle);
  if (dsa == NULL) {
    LWLockRegisterTranche(OMNI_DSA_TRANCHE, "omni:dsa");
    // It is important to allocate DSA in the top memory context
    // so it doesn't get deallocated when the context we're in is gone
    MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    dsa = dsa_create(OMNI_DSA_TRANCHE);
    MemoryContextSwitchTo(oldcontext);
    dsa_pin(dsa);
    dsa_pin_mapping(dsa);
    phandle->dsa = dsa_get_handle(dsa);
  }
  LWLockAcquire(&(locks + OMNI_LOCK_ALLOCATION)->lock,
                action == HASH_FIND ? LW_SHARED : LW_EXCLUSIVE);
  ModuleAllocationKey key = {
      .module_id = phandle->id,
  };
  strncpy(key.name, name, sizeof(key.name));
  ModuleAllocation *alloc = (ModuleAllocation *)hash_search(omni_allocations, &key, action, found);
  if (!*found) {
    if (action == HASH_ENTER || action == HASH_ENTER_NULL) {
      alloc->dsa_handle = dsa_get_handle(dsa);
      alloc->dsa_pointer = dsa_allocate(dsa, size);
      alloc->size = size;

      if (init != NULL) {
        init(dsa_get_address(dsa, alloc->dsa_pointer), data);
      }
    }
  }
  LWLockRelease(&(locks + OMNI_LOCK_ALLOCATION)->lock);
  return (
      *found || (action == HASH_ENTER || action == HASH_ENTER_NULL) // if it was found or allocated
          ? ((struct dsa_ref){.pointer = alloc->dsa_pointer, .handle = alloc->dsa_handle})
          : (struct dsa_ref){});
}

static dsa_area *dsa_handle_to_area(dsa_handle handle) {
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

MODULE_FUNCTION void *find_or_allocate_shmem(const omni_handle *handle, const char *name,
                                             size_t size, void (*init)(void *ptr, void *data),
                                             void *data, bool find, bool *found) {
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

MODULE_FUNCTION void *allocate_shmem(const omni_handle *handle, const char *name, size_t size,
                                     void (*init)(void *ptr, void *data), void *data, bool *found) {
  return find_or_allocate_shmem(handle, name, size, init, data, false, found);
}

MODULE_FUNCTION void *lookup_shmem(const omni_handle *handle, const char *name, bool *found) {
  return find_or_allocate_shmem(handle, name, 1 /* size is ignored in this case */, NULL, NULL,
                                true, found);
}

MODULE_FUNCTION void deallocate_shmem(const omni_handle *handle, const char *name, bool *found) {
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

MODULE_FUNCTION void unload_module(int64 id, bool missing_ok) {
  omni_handle_private *phandle = module_handles + id;
  LWLockAcquire(&(locks + OMNI_LOCK_MODULE)->lock, LW_EXCLUSIVE);
  bool found = false;
  ModuleEntry *module = (ModuleEntry *)hash_search(omni_modules, phandle->path, HASH_FIND, &found);
  found = found && module->id == id;
  if (!found) {
    LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);
    if (!missing_ok) {
      ereport(ERROR, errmsg("module id %lu not found", id));
    } else {
      return;
    }
  }

  // Decrement the refcount
  pg_atomic_add_fetch_u32(&phandle->refcount, -1);

  // Deinitialize this backend
  void *dlhandle = dlopen(phandle->path, RTLD_LAZY);
  void (*deinit)(const omni_handle *) = dlsym(dlhandle, "_Omni_deinit");
  if (deinit != NULL) {
    deinit(&phandle->handle);
  }
  // Exclude the module from this backend
  MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
  initialized_modules = list_delete_int(initialized_modules, id);
  MemoryContextSwitchTo(oldcontext);
  // Purge hooks
  reorganize_hooks();

  // Try to unload the module
  uint32 loaded = 1;
  // If it was found and if we were able to capture `loaded` set to `1`
  if (found && pg_atomic_compare_exchange_u32(&phandle->loaded, &loaded, 0)) {
    // Perform unloading
    void (*unload)(const omni_handle *) = dlsym(dlhandle, "_Omni_unload");
    if (unload != NULL) {
      unload(&phandle->handle);
    }
    dlclose(dlhandle);
  }
  // We no longer need the lock
  LWLockRelease(&(locks + OMNI_LOCK_MODULE)->lock);
}