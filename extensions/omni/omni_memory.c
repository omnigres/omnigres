// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include "omni_common.h"

typedef struct {
  const char *name;
  size_t size;

  char data[0];
} memory_chunk_header_t;

typedef struct {
  pg_atomic_uint64 next;

  memory_chunk_header_t header;
} memory_first_chunk_header_t;

static int omni_memory_size_mb = 0;

static memory_first_chunk_header_t *first_header;

static void *reserve_chunk(const char *name, size_t size) {
  size_t total_size = size + sizeof(memory_chunk_header_t);
  uint64 offset = pg_atomic_fetch_add_u64(&first_header->next, (size_t)total_size);
  memory_chunk_header_t *header = (memory_chunk_header_t *)(offset + (uintptr_t)first_header);
  madvise(header, total_size, MADV_NORMAL);
  header->name = name;
  header->size = size;
  return (void *)header->data;
}

static void *iterate_chunk(void *ptr) {
  if (ptr == NULL) {
    ptr = (void *)((uintptr_t)first_header + sizeof(*first_header));
  }
  memory_chunk_header_t const *hdr = (memory_chunk_header_t *)(ptr - sizeof(memory_chunk_header_t));
  size_t sz = (ptr - sizeof(memory_first_chunk_header_t) == first_header) ? 0 : hdr->size;
  void *next = ptr + sz + sizeof(memory_chunk_header_t);
  if (((uintptr_t)first_header + pg_atomic_read_u64(&first_header->next)) < (uintptr_t)next) {
    return NULL;
  }
  return next;
}

static void release_chunk(void *ptr) {
  ereport(LOG, errmsg("Releasing omni memory chunks is currently a no-op"));
}

static size_t chunk_size(void *ptr) {
  memory_chunk_header_t const *hdr = (memory_chunk_header_t *)(ptr - sizeof(memory_chunk_header_t));
  return hdr->size;
}
static const char *chunk_name(void *ptr) {
  memory_chunk_header_t const *hdr = (memory_chunk_header_t *)(ptr - sizeof(memory_chunk_header_t));
  return hdr->name;
}

MODULE_FUNCTION void initialize_omni_memory() {

  omni_memory_handle = (omni_memory_t){
      .reserve_chunk = reserve_chunk,
      .iterate_chunk = iterate_chunk,
      .release_chunk = release_chunk,
      .chunk_size = chunk_size,
      .chunk_name = chunk_name,
  };

  int protections = PROT_NONE;
  DefineCustomIntVariable("omni.omni_memory_size", "Amount of omni memory reserved",
                          "Amount of omni memory reserved to be shared between backends",
                          &omni_memory_size_mb, 0, 0, 2 ^ 64 / (1024 * 1024), PGC_POSTMASTER,
                          GUC_UNIT_MB, NULL, NULL, NULL);

  size_t omni_memory_size = 0;
  if (omni_memory_size_mb == 0) {
#ifdef __APPLE__
    size_t length = sizeof(size_t);
    sysctl((int[2]){CTL_HW, HW_MEMSIZE}, 2, &omni_memory_size, &length, NULL, 0);
#endif
#ifdef __linux__
    struct sysinfo info;
    sysinfo(&info);
    omni_memory_size = info.totalram;
#endif
    omni_memory_size_mb = (int)(omni_memory_size / (1024 * 1024));
  } else {
    omni_memory_size = omni_memory_size_mb * 1024 * 1024;
  }
  void *address;
  size_t page_size = sysconf(_SC_PAGESIZE);
  do {
    address =
        mmap(NULL, omni_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (address == MAP_FAILED) {
      size_t new_omni_memory_size = omni_memory_size / 2;
      if (new_omni_memory_size < page_size) {
        ereport(WARNING, errmsg("Failed to mmap %lu bytes for omni memory, none will be available",
                                omni_memory_size, new_omni_memory_size));
        return;
      }
      ereport(WARNING,
              errmsg("Failed to mmap %lu bytes for omni memory, attempting to allocate %lu",
                     omni_memory_size, new_omni_memory_size));
      omni_memory_size = new_omni_memory_size;
    }
  } while (address == MAP_FAILED);
  first_header = address;
  first_header->header.name = "omni_memory";
  first_header->header.size = omni_memory_size;
  pg_atomic_init_u64(&first_header->next, sizeof(*first_header));

  madvise(address + sizeof(first_header), omni_memory_size - sizeof(first_header), MADV_DONTNEED);
}
MODULE_VARIABLE(omni_memory_t omni_memory_handle);

MODULE_FUNCTION omni_memory_t *omni_memory(const omni_handle *handle) {
  return &omni_memory_handle;
}
