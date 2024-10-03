#include "libpgaug.h"
#include <sys/fcntl.h>
#define _POSIX_C_SOURCE 1
#include <limits.h>
#include <unistd.h>

#include <miscadmin.h>
#include <utils/pidfile.h>

void *pgaug_alloc(size_t size) { return palloc(size); }

void *pgaug_calloc(uintptr_t num, uintptr_t count) { return palloc0(num * count); }

void pgaug_free(void *ptr) {
  if (ptr != NULL && GetMemoryChunkContext(ptr) != NULL) {
    pfree(ptr);
  }
}

void *pgaug_realloc(void *ptr, uintptr_t size) {
  if (ptr != NULL) {
    return repalloc(ptr, size);
  } else {
    return palloc(size);
  }
}

void __with_temp_memcxt_cleanup(struct __with_temp_memcxt *s) {
  if (CurrentMemoryContext == s->new) {
    CurrentMemoryContext = s->old;
  }
  MemoryContextDelete(s->new);
}

bool IsPostmasterBeingShutdown() {
  int fd;
  int len;
  int lineno;
  char *srcptr;
  char *destptr;
  char srcbuffer[BLCKSZ];
  char destbuffer[BLCKSZ];
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "%s/postmaster.pid", DataDir);
  fd = open(path, O_RDWR | PG_BINARY, 0);
  if (fd < 0) {
    ereport(LOG, (errcode_for_file_access(), errmsg("could not open file \"%s\": %m", path)));
    return false;
  }
  len = read(fd, srcbuffer, sizeof(srcbuffer) - 1);
  if (len < 0) {
    ereport(LOG, (errcode_for_file_access(), errmsg("could not read from file \"%s\": %m", path)));
    close(fd);
    return false;
  }
  srcbuffer[len] = '\0';
  srcptr = srcbuffer;
  for (lineno = 1; lineno < LOCK_FILE_LINE_PM_STATUS; lineno++) {
    char *eol = strchr(srcptr, '\n');

    if (eol == NULL)
      return false; // can't find the necessary line, bail
    srcptr = eol + 1;
  }
  // Are we stopping?
  return (strncmp(srcptr, PM_STATUS_STOPPING, strlen(PM_STATUS_STOPPING)) == 0);
}