#ifndef apex_omni_shmem_h
#define apex_omni_shmem_h

typedef struct omni_arena_allocation {
  void *ptr;
  bool found;
} omni_arena_allocation;

typedef struct omni_arena {
  void *(*alloc)(omni_arena *, size_t size);
  void (*free)(omni_arena *, void *ptr);
  void *(*find)(omni_arena *, const char *name);
  omni_arena_allocation (*find_or_construct)(omni_arena *, const char *name, size_t size,
                                             void(destructor)(void *ptr, void *destructor_ctx),
                                             void *destructor_ctx);
  bool (*destroy)(omni_arena *, const char *name);
  void (*release)(omni_arena *);
} omni_shmem;

typedef omni_arena *(*omni_shmem_api_open)(const char *name, bool transient);
typedef omni_arena *(*omni_shmem_api_open_or_create)(const char *name, size_t size, bool transient);

typedef struct omni_shmem_api {
  omni_shmem_api_open open;
  omni_shmem_api_open_or_create open_or_create;
} omni_shmem_api;

#define apex_api_type omni_shmem_api
// TODO: we need unique API-shape name. Until APEX's script is done, we're ignoring this
#define apex_api "omni_shmem"
#include <apex.h>

#endif // apex_omni_shmem_h
