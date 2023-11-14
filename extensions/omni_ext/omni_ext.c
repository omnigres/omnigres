#include <dlfcn.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DYNPGEXT_IMPL
#include <dynpgext.h>

// clang-format off
#include <postgres.h>
#include <postmaster/bgworker.h>
// clang-format on

#include <access/heapam.h>
#include <access/sdir.h>
#include <access/table.h>
#include <access/tableam.h>
#include <catalog/pg_database.h>
#include <miscadmin.h>
#include <storage/fd.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/dsa.h>
#include <utils/guc.h>
#include <utils/hsearch.h>

#include <libgluepg_stc.h>
#include <libgluepg_stc/str.h>
#include <libgluepg_stc/sview.h>

#include "control_file.h"
#include "omni_ext.h"
#include "strverscmp.h"

static const char *find_absolute_library_path(const char *filename) {
  const char *result = filename;
#ifdef __linux__
  // Not a great solution, but not aware of anything else yet.
  // This code below reads /proc/self/maps and finds the path to the
  // library by matching the base address of omni_ext shared library.

  FILE *f = fopen("/proc/self/maps", "r");
  if (NULL == f) {
    return result;
  }

  // Get the base address of omni_ext shared library
  Dl_info info;
  dladdr(get_library_name, &info);

  // We can keep this name around forever as it'll be used to create handles
  char *path = MemoryContextAllocZero(TopMemoryContext, NAME_MAX + 1);
  char *format = psprintf("%%lx-%%*x %%*s %%*s %%*s %%*s %%%d[^\n]", NAME_MAX);

  uintptr_t base;
  while (fscanf(f, (const char *)format, &base, path) >= 1) {
    if (base == (uintptr_t)info.dli_fbase) {
      result = path;
      goto done;
    }
  }
done:
  pfree(format);
  fclose(f);
#endif
  return result;
}

/**
 * @brief Get path to omni_ext's library shared object
 *
 * This is to be primarily used by omni_ext's workers.
 *
 * @return const char*
 */
const char *get_library_name() {
  static const char *library_name = NULL;
  // If we have already determined the name, return it
  if (library_name) {
    return library_name;
  }
#ifdef HAVE_DLADDR
  Dl_info info;
  dladdr(get_library_name, &info);
  library_name = info.dli_fname;
  if (index(library_name, '/') == NULL) {
    // Not a full path, try to determine it. On some systems it will be a full path, on some it
    // won't.
    library_name = find_absolute_library_path(library_name);
  }
#else
  library_name = EXT_LIBRARY_NAME;
#endif
  return library_name;
}

bool dsa_attached = false;

void ensure_dsa_attached() {
  if (!dsa_attached) {
    // Ensure dsa_area is allocated in top memory context, otherwise
    // it's going to get freed with the current memory context.
    MemoryContext current = MemoryContextSwitchTo(TopMemoryContext);
    My_dsa_area = dsa_attach_in_place(My_dsa_mem, NULL);
    dsa_pin_mapping(My_dsa_area);
    dsa_attached = true;
    MemoryContextSwitchTo(current);
  }
}

int shmem_size;
int max_databases;
int max_allocation_dictionary_entries;
dsa_area *My_dsa_area;
void *My_dsa_mem;
pid_t My_dsa_pid;
cdeq_allocation_request allocation_requests;
cdeq_background_worker_request background_worker_requests;
cdeq_handle handles;

HASHCTL allocation_dictionary_ctl = {.keysize = ALLOCATION_DICTIONARY_KEY_SIZE,
                                     .entrysize = sizeof(allocation_dictionary_entry)};

HASHCTL database_oid_mapping_ctl = {.keysize = sizeof(Oid),
                                    .entrysize = sizeof(database_oid_mapping_entry)};

int allocation_request_cmp(const allocation_request *left, const allocation_request *right) {
  if (left->callback == right->callback && left->data == right->data) {
    return 0;
  } else if (left->size <= right->size) {
    return -1;
  } else if (left->size > right->size) {
    return 1;
  } else {
    return -1;
  }
}

int background_worker_request_cmp(const background_worker_request *left,
                                  const background_worker_request *right) {
  return -1;
}

int dynpgext_handle_cmp(dynpgext_handle *const *left, dynpgext_handle *const *right) {
  int name_cmp = strcmp((*left)->name, (*right)->name);
  if (name_cmp != 0) {
    return name_cmp;
  }
  int version_cmp = strcmp((*left)->version, (*right)->version);
  if (version_cmp != 0) {
    return version_cmp;
  }
  int library_cmp = strcmp((*left)->library_name, (*right)->library_name);
  return library_cmp;
}

void allocate_shmem_startup(const dynpgext_handle *handle, const char *name, size_t size,
                            void (*callback)(void *ptr, void *data), void *data,
                            dynpgext_allocate_shmem_flags flags) {
  cdeq_allocation_request_push_back(&allocation_requests, (allocation_request){.handle = handle,
                                                                               .size = size,
                                                                               .callback = callback,
                                                                               .data = data,
                                                                               .flags = flags,
                                                                               .name = name});
}

void register_bgworker_startup(const dynpgext_handle *handle, BackgroundWorker *bgw,
                               void (*callback)(BackgroundWorkerHandle *handle, void *data),
                               void *data, dynpgext_register_bgworker_flags flags) {

  if (bgw->bgw_notify_pid == MyProcPid) {
    // Regardless of the PID of the process that will actually be registering the worker,
    // we'll know the extension wanted to be notified
    flags |= DYNPGEXT_REGISTER_BGWORKER_NOTIFY;
  }
  background_worker_request request = {
      .handle = handle, .callback = callback, .data = data, .flags = flags};
  memcpy(&request.bgw, bgw, sizeof(BackgroundWorker));
  cdeq_background_worker_request_push_back(&background_worker_requests, request);
}

void allocate_shmem_runtime(const dynpgext_handle *handle, const char *name, size_t size,
                            void (*callback)(void *ptr, void *data), void *data,
                            dynpgext_allocate_shmem_flags flags) {
  HTAB *dict = ShmemInitHash("omni_ext_allocation_dictionary",
                             cdeq_allocation_request_size(&allocation_requests),
                             max_allocation_dictionary_entries, &allocation_dictionary_ctl,
                             HASH_ELEM | HASH_STRINGS | HASH_ATTACH);

  LWLock *lock = &(GetNamedLWLockTranche("omni_ext_allocation_dictionary")->lock);

  int multiplier = 1;
  if ((flags & DYNPGEXT_SCOPE_DATABASE_LOCAL) == DYNPGEXT_SCOPE_DATABASE_LOCAL) {
    multiplier = max_databases;
  }
  size_t size_ = size * multiplier;
  if (size_ >= 1 * 1024 * 1024 * 1024) {
    flags = DSA_ALLOC_HUGE;
  }
  ensure_dsa_attached();
  dsa_pointer ptr = dsa_allocate_extended(My_dsa_area, size_, flags);
  LWLockAcquire(lock, LW_EXCLUSIVE);
  bool found = false;
  allocation_dictionary_entry *entry =
      (allocation_dictionary_entry *)hash_search(dict, name, HASH_ENTER, &found);
  if (!found) {
    entry->ptr = ptr;
    entry->flags = flags;
    entry->size = size;
  }
  LWLockRelease(lock);

  if (callback) {
    void *addr = dsa_get_address(My_dsa_area, ptr);
    for (int i = 0; i < multiplier; i++) {
      callback(addr + (i * size), data);
    }
  }
}

void register_bgworker_runtime(const dynpgext_handle *handle, BackgroundWorker *bgw,
                               void (*callback)(BackgroundWorkerHandle *handle, void *data),
                               void *data, dynpgext_register_bgworker_flags flags) {
  if ((flags & DYNPGEXT_REGISTER_BGWORKER_NOTIFY) == DYNPGEXT_REGISTER_BGWORKER_NOTIFY) {
    bgw->bgw_notify_pid = MyProcPid;
  }
  BackgroundWorkerHandle *worker_handle;
  if ((flags & DYNPGEXT_SCOPE_DATABASE_LOCAL) == DYNPGEXT_SCOPE_DATABASE_LOCAL) {
    // TODO: communicate with workers to start database-local workers
  } else {
    RegisterDynamicBackgroundWorker(bgw, &worker_handle);
    if (callback) {
      callback(worker_handle, data);
    }
  }
}

void *dynpgext_lookup_shmem(const char *name) {
  HTAB *dict = ShmemInitHash("omni_ext_allocation_dictionary",
                             cdeq_allocation_request_size(&allocation_requests),
                             max_allocation_dictionary_entries, &allocation_dictionary_ctl,
                             HASH_ELEM | HASH_STRINGS | HASH_ATTACH);
  LWLock *lock = &(GetNamedLWLockTranche("omni_ext_allocation_dictionary")->lock);
  LWLockAcquire(lock, LW_SHARED);
  bool found = false;
  allocation_dictionary_entry *entry =
      (allocation_dictionary_entry *)hash_search(dict, name, HASH_FIND, &found);
  void *result = NULL;
  if (found) {
    ensure_dsa_attached();
    result = dsa_get_address(My_dsa_area, entry->ptr);
    if ((entry->flags & DYNPGEXT_SCOPE_DATABASE_LOCAL) == DYNPGEXT_SCOPE_DATABASE_LOCAL) {
      HTAB *mappings =
          ShmemInitHash("omni_ext_database_oid_mapping", 0, max_databases,
                        &database_oid_mapping_ctl, HASH_ELEM | HASH_BLOBS | HASH_ATTACH);
      LWLock *mapping_lock = &(GetNamedLWLockTranche("omni_ext_database_oid_mapping")->lock);
      // TODO: avoid relying on lock exclusivity in all scenarios, it's an unnecessary bottleneck
      LWLockAcquire(mapping_lock, LW_EXCLUSIVE);
      bool found_mapping = false;
      database_oid_mapping_entry *mapping = (database_oid_mapping_entry *)hash_search(
          mappings, &MyDatabaseId, HASH_ENTER, &found_mapping);
      if (found_mapping) {
        result += mapping->index * entry->size;
      } else {
        long entry_n = hash_get_num_entries(mappings) - 1;
        mapping->index = entry_n;
        result += mapping->index * entry->size;
      }
      LWLockRelease(mapping_lock);
    }
  }
  LWLockRelease(lock);
  return result;
}

PG_FUNCTION_INFO_V1(load);

struct matching_control_file_search {
  char *name;
  char *version;
  char *exact_match;
  char *unversioned_match;
  char *versioned_match;
  char *versioned_match_version;
};

void pick_matching_control_file(const char *control_file, void *data) {
  struct matching_control_file_search *search = (struct matching_control_file_search *)data;

  char *control_basename = basename((char *)control_file);
  char *version = NULL;
  char *name = NULL;
  // Split off the filename extension
  csview filename =
      csview_from_n(control_basename, (strrchr(control_basename, '.') - control_basename));
  size_t version_sep_pos = csview_find(filename, "--");
  if (strncmp(filename.str, search->name,
              Max(version_sep_pos == c_NPOS ? filename.size : version_sep_pos,
                  strlen(search->name))) != 0) {
    // This is a different extension, skip it
    return;
  }

  if (version_sep_pos == c_NPOS) {
    // No version supplied
    search->unversioned_match = pstrdup(control_file);
  } else {
    // Version is supplied
    csview ext_version = csview_substr(filename, version_sep_pos + 2, csview_size(filename));
    char *version = pstring_from_csview(ext_version);
    if (search->version && strncmp(ext_version.str, search->version, ext_version.size) == 0) {
      search->exact_match = pstrdup(control_file);
    } else if (!search->version) {
      if (!search->versioned_match) {
        // No versioned matches picked yet
        search->versioned_match = pstrdup(control_file);
        search->versioned_match_version = version;
      } else {
        if (strverscmp(search->versioned_match_version, version) < 0) {

          // We found an older version, replace it with a newer one
          search->versioned_match = pstrdup(control_file);
          search->versioned_match_version = version;
        }
      }
    }
  }
}

Datum load(PG_FUNCTION_ARGS) {
  char *name = NULL;
  if (!PG_ARGISNULL(0)) {
    name = PG_GETARG_CSTRING(0);
  } else {
    ereport(ERROR, errmsg("extension name is required"));
  }
  char *version = NULL;
  if (!PG_ARGISNULL(1)) {
    version = PG_GETARG_CSTRING(1);
  }

  struct matching_control_file_search search = {
      .name = name, .version = version, .exact_match = NULL, .unversioned_match = NULL};

  char *result;
  Datum value;

  struct load_control_file_config config = {.preload = false,
                                            .action = LOAD,
                                            .allocate_shmem = allocate_shmem_runtime,
                                            .expected_version = version,
                                            .register_bgworker_function =
                                                register_bgworker_runtime};

  WITH_TEMP_MEMCXT {
    find_control_files(pick_matching_control_file, &search);

    if (version && search.exact_match) {
      load_control_file(search.exact_match, (void *)&config);
      result = version;
    } else if (version && search.unversioned_match) {
      // FIXME: should we only load this if `default_version` is matching?
      load_control_file(search.unversioned_match, (void *)&config);
      result = "";
    } else if (!version && search.versioned_match) {
      load_control_file(search.versioned_match, (void *)&config);
      result = search.versioned_match_version;
    } else if (!version && search.unversioned_match) {
      load_control_file(search.unversioned_match, (void *)&config);
      result = "";
    } else {
      ereport(ERROR, errmsg("No matching control file found"));
    }
  }
  MEMCXT_FINALIZE { value = CStringGetDatum(pstrdup(result)); };
  return value;
}

PG_FUNCTION_INFO_V1(unload);
Datum unload(PG_FUNCTION_ARGS) {
  char *name = NULL;
  if (!PG_ARGISNULL(0)) {
    name = PG_GETARG_CSTRING(0);
  } else {
    ereport(ERROR, errmsg("extension name is required"));
  }
  char *version = NULL;
  if (!PG_ARGISNULL(1)) {
    version = PG_GETARG_CSTRING(1);
  } else {
    ereport(ERROR, errmsg("extension version is required"));
  }

  struct matching_control_file_search search = {
      .name = name, .version = version, .exact_match = NULL, .unversioned_match = NULL};

  Datum value;

  struct load_control_file_config config = {.preload = false,
                                            .action = UNLOAD,
                                            .allocate_shmem = allocate_shmem_runtime,
                                            .expected_version = version,
                                            .register_bgworker_function =
                                                register_bgworker_runtime};

  bool result = false;
  WITH_TEMP_MEMCXT {
    find_control_files(pick_matching_control_file, &search);

    if (version && search.exact_match) {
      load_control_file(search.exact_match, (void *)&config);
      result = true;
    } else if (version && search.unversioned_match) {
      // FIXME: should we only load this if `default_version` is matching?
      load_control_file(search.unversioned_match, (void *)&config);
      result = true;
    } else {
      ereport(ERROR, errmsg("No matching control file found"));
    }
  }
  // TODO: free allocated resources (shmem & workers)? or should this be a responsibility
  // of the extension to ensure this is done exactly how they need it?
  PG_RETURN_BOOL(result);
}