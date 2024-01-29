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

#include <omni.h>

#include "omni_common.h"

PG_MODULE_MAGIC;
OMNI_MAGIC;

omni_shared_info *shared_info;

HTAB *omni_modules;
omni_handle_private *module_handles;

LWLock *locks;

static TupleDesc pg_proc_tuple_desc;
static List *initialized_modules = NIL;

static inline void ensure_backend_initialized(void) {
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

  locks = &GetNamedLWLockTranche("omni")->lock;

  backend_initialized = true;
}

static void register_hook(const omni_handle *handle, omni_hook *hook);
static char *get_library_name(const omni_handle *handle);

static int32 last_known_module = 0;

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
          if (magic->size == sizeof(omni_magic) && magic->version == 0) {
            // We are going to record it if it wasn't yet
            LWLockAcquire(locks + OMNI_LOCK_MODULE, LW_EXCLUSIVE);
            bool found = false;
            ModuleEntry *entry = hash_search(omni_modules, key, HASH_ENTER, &found);

            if (!found) {
              uint32 id = pg_atomic_add_fetch_u32(&shared_info->module_counter, 1);
              last_known_module = id;
              omni_handle_private *handle = module_handles + id;
              // If not found, prepare the handle
              handle->id = id;
              pg_atomic_init_u32(&handle->state, HANDLE_LOADED);
              strcpy(handle->path, key);
              handle->handle.register_hook = register_hook;
              // FIXME: this may not mean anything on other backends:
              handle->handle.get_library_name = get_library_name;
              entry->id = id;
              // Let's also load it if there's a callback
              void (*load_fn)(const omni_handle *) = dlsym(dlhandle, "_Omni_load");
              if (load_fn != NULL) {
                load_fn(&handle->handle);
              }
            }

            loaded = list_append_unique_int(loaded, entry->id);

            {
              // If the module was not initialized in this backend, do so
              if (!list_member_int(initialized_modules, entry->id)) {
                void (*init_fn)(const omni_handle *) = dlsym(dlhandle, "_Omni_init");
                if (init_fn != NULL) {
                  omni_handle_private *handle = module_handles + entry->id;
                  init_fn(&handle->handle);
                }
                MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
                initialized_modules = list_append_unique_int(initialized_modules, entry->id);
                MemoryContextSwitchTo(oldcontext);
              }
            }

            LWLockRelease(locks + OMNI_LOCK_MODULE);
          } else {
            ereport(WARNING, errmsg("Incompatible magic version %d (expected 0)", magic->version));
          }
        }
        dlclose(dlhandle);
      }
    }
  }
  return loaded;
}

static inline void load_module_if_necessary(Oid fn_oid, bool force_reload) {
  static bool first_time = true;

  if (first_time || force_reload || backend_force_reload) {
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

  int counter = pg_atomic_read_u32(&shared_info->module_counter);
  if (counter > last_known_module) {
    for (last_known_module++; last_known_module <= counter; last_known_module++) {
      omni_handle_private *phandle = &module_handles[last_known_module];

      bool proceed = false;
      {
        // Figure out if there is a pg_proc record referencing this path in this database
        // If there isn't any, don't try to initialize it
        Relation rel = table_open(ProcedureRelationId, RowShareLock);
        TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
        for (;;) {
          HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
          if (tup == NULL)
            break;

          Form_pg_proc proc = (Form_pg_proc)GETSTRUCT(tup);
          if (proc->prolang == ClanguageId) {
            bool isnull;
            Datum probin = heap_getattr(tup, Anum_pg_proc_probin, pg_proc_tuple_desc, &isnull);
            if (!isnull) {
              if (strcmp(text_to_cstring(DatumGetTextPP(probin)), phandle->path) == 0) {
                proceed = true;
                break;
              }
            }
          }
        }
        if (scan->rs_rd->rd_tableam->scan_end) {
          scan->rs_rd->rd_tableam->scan_end(scan);
        }
        table_close(rel, RowShareLock);
      }

      if (!proceed) {
        // Go on to the next module, if any
        continue;
      }

      void *dlhandle = dlopen(phandle->path, RTLD_LAZY);
      if (!list_member_int(initialized_modules, phandle->id)) {
        void (*init_fn)(const omni_handle *) = dlsym(dlhandle, "_Omni_init");
        if (init_fn != NULL) {
          init_fn(&phandle->handle);
        }
        MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
        initialized_modules = list_append_unique_int(initialized_modules, phandle->id);
        MemoryContextSwitchTo(oldcontext);
      }
    }
  }
}

static void register_hook(const omni_handle *handle, omni_hook *hook) {
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

static char *get_library_name(const omni_handle *handle) {
  return struct_from_member(omni_handle_private, handle, handle)->path;
}

void unload_module(int64 id, bool missing_ok) {
  omni_handle_private *phandle = module_handles + id;
  LWLockAcquire(locks + OMNI_LOCK_MODULE, LW_EXCLUSIVE);
  bool found = false;
  hash_search(omni_modules, phandle->path, HASH_REMOVE, &found);
  if (!found) {
    LWLockRelease(locks + OMNI_LOCK_MODULE);
    if (!missing_ok) {
      ereport(ERROR, errmsg("module id %lu not found", id));
    } else {
      return;
    }
  }
  pg_atomic_write_u32(&phandle->state, HANDLE_UNLOADED);
  {
    // Perform unloading
    void *dlhandle = dlopen(phandle->path, RTLD_LAZY);
    void (*unload)(const omni_handle *) = dlsym(dlhandle, "_Omni_unload");
    if (unload != NULL) {
      unload(&phandle->handle);
    }
    dlclose(dlhandle);
  }

  MemoryContext oldcontext = MemoryContextSwitchTo(TopMemoryContext);
  initialized_modules = list_delete_int(initialized_modules, id);
  MemoryContextSwitchTo(oldcontext);
  LWLockRelease(locks + OMNI_LOCK_MODULE);
}