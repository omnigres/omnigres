/*! \file */
#ifndef DYNPGEXT_H
#define DYNPGEXT_H

#include <dlfcn.h>
#include <stddef.h>

// clang-format off
#include <postgres.h>
#include <postmaster/bgworker.h>
// clang-format on

/**
 * @private
 * @brief Magic structure for compatibility checks
 */
typedef struct {
  size_t size;
  uint8_t version;
} dynpgext_magic;

typedef struct dynpgext_handle dynpgext_handle;

/**
 * @brief Extension initialization callback
 *
 * Defined by the extension. Optional but highly recommended.
 *
 * @param handle Loader handle
 */
void _Dynpgext_init(const dynpgext_handle *handle);
/**
 * @brief Extension deinitialization callback
 *
 * Defined by the extension. Optional.
 *
 * @param handle Loader handle
 */
void _Dynpgext_fini(const dynpgext_handle *handle);

/**
 * @brief If defined, can advise the loader whether this extension is to be preloaded upon startup
 *
 * If it returns false, it'll be only loaded explicitly.
 *
 * Defined by the extension. Optional. If undefined, the extension is assumed to return `true`
 *
 * @return true
 * @return false
 */
bool _Dynpgext_eager_preload();

/**
 * @brief Every dynamic extension should use this macro in addition to `PG_MODULE_MAGIC`
 *
 */
#define DYNPGEXT_MAGIC                                                                             \
  static dynpgext_magic __Dynpgext_magic = {.size = sizeof(dynpgext_magic), .version = 0};         \
  dynpgext_magic *_Dynpgext_magic() { return &__Dynpgext_magic; }

/**
 * @brief Scoped globally for the entire Postgres instance
 * For shared memory allocations, #dynpgext_lookup_shmem will return the same allocation regardless
 of
 * `MyDatabase` (current database)
 *
 * For background workers, this means that as long as there's at least one database that has the
 * extension created, one global instance of the background worker will be started.
 *
 */
#define DYNPGEXT_SCOPE_GLOBAL 0b00000000
/**
 * @brief Scoped for each individual database
 *
 * For shared memory allocations, #dynpgext_lookup_shmem will return a different allocation based on
 * `MyDatabase` (current database)
 *
 * For background workers, this means that for each database where the extension is created, a
 * worker will be started.
 *
 */
#define DYNPGEXT_SCOPE_DATABASE_LOCAL 0b000000001
/**
 * @brief Default allocation flags
 *
 * At the moment, same as #DYNPGEXT_SCOPE_GLOBAL
 *
 */
#define DYNPGEXT_ALLOCATE_SHMEM_DEFAULTS DYNPGEXT_SCOPE_GLOBAL

/**
 * @brief Shared memory allocation flags
 *
 * Allowed values:
 *
 * #DYNPGEXT_SCOPE_GLOBAL
 * #DYNPGEXT_SCOPE_DATABASE_LOCAL
 * #DYNPGEXT_ALLOCATE_SHMEM_DEFAULTS
 *
 */
typedef int dynpgext_allocate_shmem_flags;

/**
 * @brief Notify the extension about the status of the worker
 *
 * Setting this flag ensures the callback the extension receives will be able to
 * use `WaitForBackgroundWorkerStartup`.
 *
 * This can also be achieved by setting `bgw_notify_pid` to `MyProcPid`
 *
 */
#define DYNPGEXT_REGISTER_BGWORKER_NOTIFY 0b00000010
/**
 * @brief Default background worker flags
 *
 * At the moment, same as #DYNPGEXT_SCOPE_DATABASE_LOCAL
 *
 */
#define DYNPGEXT_REGISTER_BGWORKER_DEFAULTS DYNPGEXT_SCOPE_DATABASE_LOCAL

/**
 * @brief Background worker registration flags
 *
 * Allowed values:
 *
 * #DYNPGEXT_SCOPE_GLOBAL
 * #DYNPGEXT_SCOPE_DATABASE_LOCAL
 * #DYNPGEXT_REGISTER_BGWORKER_NOTIFY
 * #DYNPGEXT_REGISTER_BGWORKER_DEFAULTS
 *
 */
typedef int dynpgext_register_bgworker_flags;

/**
 * @brief Shared memory allocation function
 *
 * @param handle Handle passed by the loader
 * @param name Name to register this allocation under. It is advised to include
 *             version information into the name to facilitate easier upgrades
 * @param size Amount of memory to allocate
 * @param callback Callback to initialize the allocated memory. Can be `NULL`.
 *                 If defined, it'll be called after the memory is allocated.
 *                 The loader may choose to call it immediately after the allocation,
 *                 or at any other time, but it is guaranteed to be called before
 *                 a call to #dynpgext_lookup_shmem returns for this particular allocation.
 *                 This allows for lazy memory initialization.
 * @param data Opaque pointer to data to be passed to the callback.
 *             Must be valid until after the callback is called.
 * @param flags Allocation flags
 *
 */
typedef void (*dynpgext_allocate_shmem_function)(const dynpgext_handle *handle, const char *name,
                                                 size_t size,
                                                 void (*callback)(void *ptr, void *data),
                                                 void *data, dynpgext_allocate_shmem_flags flags);

/**
 * @brief Background worker registration function
 *
 */
typedef void (*dynpgext_register_bgworker_function)(
    const dynpgext_handle *handle, BackgroundWorker *bgw,
    void (*callback)(BackgroundWorkerHandle *handle, void *data), void *data,
    dynpgext_register_bgworker_flags flags);

/**
 * @brief Handle provided by the loader
 *
 */
typedef struct dynpgext_handle {
  /**
   * @brief Extension name
   *
   */
  char *name;
  /**
   * @brief Extension version
   *
   */
  char *version;
  /**
   * @brief Shared library (.so) name
   *
   * Can be used to register background worker from the same library.
   *
   */
  char *library_name;
  /**
   * @brief Shared memory allocation function
   *
   */
  dynpgext_allocate_shmem_function allocate_shmem;
  /**
   * @brief Background worker registration function
   *
   */
  dynpgext_register_bgworker_function register_bgworker;
} dynpgext_handle;

/**
 * @brief Lookup previously allocated shared memory
 *
 * This function is defined by the loader.
 *
 * @param name name it was registered under
 * @return void* pointer to the allocation, NULL if none found
 */
#if defined(__APPLE__) && !defined(DYNPGEXT_IMPL)
// This is a workaround for macOS' linker requiring the symbol available during linking.
// Instead of setting `-undefined dynamic_lookup`, do this dynamic lookup ourselves and cache it
// at the call site or globally (using a `static` variable).
// If call site caching is sufficient, one can use this as is or define `DYNPGEXT_SUPPLEMENTARY`
// to silence the warning.
#if defined(DYNPGEXT_MAIN)
void *(*_dynpgext_lookup_shmem)(const char *) = NULL;
#elif !defined(DYNPGEXT_SUPPLEMENTARY)

#warning "_dynpgext_lookup_shmem is declared static, this will lead to a performance penalty, \
add `#define DYNPGEXT_MAIN` in one of the translation units in the extension before #include \
<dynpgext.h> \ and `#define DYNPGEXT_SUPPLEMENTARY` in others to improve performance."

static void *(*_dynpgext_lookup_shmem)(const char *);
#else
extern void *(*_dynpgext_lookup_shmem)(const char *);
#endif

__attribute__((always_inline)) inline static void *dynpgext_lookup_shmem(const char *name) {
  if (!_dynpgext_lookup_shmem)
    _dynpgext_lookup_shmem = dlsym(RTLD_DEFAULT, "dynpgext_lookup_shmem");
  return _dynpgext_lookup_shmem(name);
}

#else
extern void *dynpgext_lookup_shmem(const char *name);
#endif

#endif // DYNPGEXT_H