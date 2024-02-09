// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <dlfcn.h>
#include <unistd.h>

#include "omni_common.h"

/**
 * Returns a name that fits into BGWLEN-1
 *
 * Many pathnames don't. So we try to do this by symlinking into $TMPDIR
 * hoping it'll be shorter.
 *
 * (bgw_library_name should be MAXPGPATH-sized, really)
 *
 * @param library_name
 * @return
 */
MODULE_FUNCTION char *get_fitting_library_name(char *library_name) {
  if (sizeof(((BackgroundWorker){}).bgw_library_name) == BGW_MAXLEN &&
      strlen(library_name) >= BGW_MAXLEN - 1) {
    char *tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL) {
      ereport(WARNING, errmsg("library path %s is too long to fit into BGW_MAXLEN-1 (%d chars) and "
                              "there's no $TMPDIR",
                              library_name, BGW_MAXLEN - 1));
    } else {
      char *tempfile = psprintf("%s/omni_ext_XXXXXX", tmpdir);
      if (strlen(tempfile) >= BGW_MAXLEN - 1) {
        ereport(WARNING,
                errmsg("temp file name %s is still to large to fit into BGW_MAXLEN-1 (%d chars)",
                       tempfile, BGW_MAXLEN));
        return library_name;
      }
      int fd = mkstemp(tempfile);
      unlink(tempfile);
      close(fd);
      if (symlink(library_name, tempfile) != 0) {
        int e = errno;
        ereport(WARNING, errmsg("can't symlink %s to %s: %s", library_name, tempfile, strerror(e)));
        return library_name;
      }
      return tempfile;
    }
  }
  return library_name;
}

MODULE_FUNCTION const char *find_absolute_library_path(const char *filename) {
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
 * @brief Get path to omni's library shared object
 *
 * This is to be primarily used by omni_ext's workers.
 *
 * @return const char*
 */
MODULE_FUNCTION const char *get_omni_library_name() {
  const char *library_name = NULL;
  // If we have already determined the name, return it
  if (library_name) {
    return library_name;
  }
#ifdef HAVE_DLADDR
  Dl_info info;
  dladdr(_Omni_init, &info);
  library_name = info.dli_fname;
  if (index(library_name, '/') == NULL) {
    // Not a full path, try to determine it. On some systems it will be a full path, on some it
    // won't.
    library_name = find_absolute_library_path(library_name);
  }
#else
  library_name = EXT_LIBRARY_NAME;
#endif
  library_name = get_fitting_library_name((char *)library_name);
  return library_name;
}