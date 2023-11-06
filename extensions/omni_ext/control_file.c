#include <dlfcn.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <dynpgext.h>

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

#ifndef DLSUFFIX
#define DLSUFFIX ".so"
#endif

#define MAX_EXTKEYSIZE 1024

/**
 * @brief Check if file exists and is accessible permissions-wise
 *
 * File must be a regular file.
 *
 * @param name filename
 * @return true file exists and is accessible
 * @return false file does not exist or is not accessible
 */
static bool file_accessible(const char *name) {
  struct stat fstat;
  stat(name, &fstat);
  return access(name, F_OK) == 0 && S_ISREG(fstat.st_mode);
}

/**
 * @brief Expands $libpath macro in csview
 *
 * @param name file path
 * @return cstr expanded path
 */
static cstr expand_libpath_macro_cstr(csview name) {
  cstr str = cstr_from_sv(name);
  cstr_replace(&str, "$libdir", pkglib_path);
  return str;
}

/**
 * @brief Expands $libpath macro in a string
 *
 * @param name file path
 * @return char* expanded path
 */
char *expand_libpath_macro(const char *name) {
  Assert(name != NULL);
  cstr str = expand_libpath_macro_cstr(csview_from(name));
  char *result = pstrdup(cstr_data(&str));
  cstr_drop(&str);
  return result;
}

#ifdef WIN32
#define PATH_SEPARATOR ";"
#else
#define PATH_SEPARATOR ":"
#endif

/**
 * @brief Finds first accessible file in path
 *
 * @param name file to find
 * @param input separated list of directories (';'-separated on Windows, ':'
 * otherwise)
 * @return char * pallocated valid file path
 */
static char *file_in_path(const char *name, csview input) {
  size_t pos = 0;
  while (pos <= input.size) {
    csview tok = csview_token(input, PATH_SEPARATOR, &pos);
    cstr s = expand_libpath_macro_cstr(tok);
    char *absolute_path = make_absolute_path(cstr_data(&s));
    char *path = psprintf("%s/%s", absolute_path, name);
    // make_absolute_path mallocs
    free(absolute_path);
    if (file_accessible(path)) {
      return path;
    }
  }
  return pstrdup(name);
}

char *find_library_in_dynamic_libpath(const char *name) {
  Assert(Dynamic_library_path != NULL);

  cstr str = cstr_from(Dynamic_library_path);

  return file_in_path(name, csview_from(Dynamic_library_path));
}

static char *dynamic_library_name(const char *name) {
  Assert(name);

  char *filename = (char *)name;
  char *resolved = NULL;

  while (true) {
    bool has_path = first_dir_separator(filename) != NULL;
    if (!has_path) {
      resolved = find_library_in_dynamic_libpath(filename);
      if (resolved) {
        break;
      }
    } else {
      resolved = expand_libpath_macro((const char *)filename);
      if (file_accessible((const char *)resolved)) {
        break;
      } else {
        pfree(resolved);
      }
    }

    // Try again with the dynamic library suffix if `filename` still
    // points to `name`
    if (filename == name) {
      filename = psprintf("%s%s", name, DLSUFFIX);
    } else {
      break;
    }
  }
  // If `filename` no longer points to `name`, free it as we allocated it
  if (filename != name) {
    pfree(filename);
  }
  return (resolved != NULL && strlen(resolved) == 0) ? NULL : resolved;
}

void find_control_files(void (*callback)(const char *control_path, void *data), void *data) {
  // Find path to extension control files
  char extension_path[MAXPGPATH];
  get_share_path((const char *)&my_exec_path, extension_path);
  // If we can't fit "extension/" into path, bail
  if (strlen(extension_path) + 10 > MAXPGPATH - 1) {
    ereport(ERROR, errmsg("Share path %s is too long", extension_path));
  }
  strcpy(extension_path + strlen(extension_path), "/extension");

  // Scan the directory
  DIR *dir = opendir(extension_path);
  int err;
  if (!dir) {
    err = errno;
    ereport(ERROR, errmsg("Error opening extension directory %s", extension_path),
            errdetail("%s", strerror(err)));
  }
  char control_path[MAXPGPATH];
  while (dir) {
    struct dirent *ent = readdir(dir);
    if (ent != NULL) {
      char *ext = strrchr(ent->d_name, '.');
      // Identify a control file
      if (strcmp(ext, ".control") == 0) {
        if (strlen(extension_path) + strlen(ent->d_name) + 1 > MAXPGPATH - 1) {
          ereport(ERROR,
                  errmsg("Control file path %s/%s is too long", extension_path, ent->d_name));
        }

        // Prepare full control file path
        strcpy(control_path, extension_path);
        control_path[strlen(extension_path)] = '/';
        strcpy(control_path + strlen(extension_path) + 1, ent->d_name);

        callback((const char *)control_path, data);
      }
    } else {
      err = errno;
      break;
      if (err != 0) {
        ereport(ERROR, errmsg("Error reading extension directory %s", extension_path),
                errdetail("%s", strerror(err)));
      }
    }
  }
  closedir(dir);
}

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
static char *get_fitting_library_name(char *library_name) {
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

void load_control_file(const char *control_path, void *data) {
  char *control_basename = basename((char *)control_path);
  struct load_control_file_config *config = (struct load_control_file_config *)data;

  WITH_TEMP_MEMCXT {
    FILE *fp = AllocateFile(control_path, "r");
    if (fp) {
      ConfigVariable *head = NULL, *tail = NULL;
      if (ParseConfigFp(fp, control_path, 0, FATAL, &head, &tail)) {
        // Control file parsed successfully
        ConfigVariable *current = head;
        control_file control_file = {.directory = "extension"};
        // Prepare extension entry
        {
          // Split off the extension
          csview filename =
              csview_from_n(control_basename, (strrchr(control_basename, '.') - control_basename));
          size_t version_sep_pos = csview_find(filename, "--");
          if (version_sep_pos == c_NPOS) {
            // No version supplied
            control_file.ext_name = pstring_from_csview(filename);
            control_file.ext_version = NULL;
          } else {
            csview ext_name = csview_substr(filename, 0, version_sep_pos);
            control_file.ext_name = pstring_from_csview(ext_name);
            // Version is supplied
            csview ext_version =
                csview_substr(filename, version_sep_pos + 2, csview_size(filename));
            control_file.ext_version = pstring_from_csview(ext_version);
          }
        }
        while (current) {
          if (!current->ignore) {
            if (strcmp(current->name, "module_pathname") == 0) {
              control_file.module_pathname = dynamic_library_name(current->value);
            } else if (control_file.ext_version == NULL &&
                       strcmp(current->name, "default_version") == 0) {
              control_file.default_version = current->value;
              control_file.ext_version = current->value;
            }
            // TODO: parse more fields
          }
          current = current->next;
        }
        FreeFile(fp);

        bool matching_version = config->expected_version == NULL ||
                                strcmp(config->expected_version, control_file.ext_version) == 0;
        // Only proceed if there's a module pathname and a matching version
        bool proceed = (control_file.module_pathname != NULL) && matching_version;
        // Check if this extension was already loaded at the statup time
        if (!config->preload) {
          c_FOREACH(it, cdeq_handle, handles) {
            // and if it was, don't proceed with it
            proceed = !(strcmp((*it.ref)->name, control_file.ext_name) == 0 &&
                        strcmp((*it.ref)->version, control_file.ext_version) == 0);

            if (!proceed)
              break;
          }
        }

        // Try loading the extension
        if (proceed) {
          void *ext_handle = dlopen(control_file.module_pathname, RTLD_NOW);
          if (ext_handle == NULL) {
            ereport(WARNING,
                    errmsg("Failed loading extension %s: %s", control_file.ext_name, dlerror()));
          } else {
            // Check if magic callback is present
            dynpgext_magic *(*magic_fn)() = dlsym(ext_handle, "_Dynpgext_magic");
            if (magic_fn != NULL) {
              // Check if magic is correct
              dynpgext_magic *magic = magic_fn();
              if (magic->size == sizeof(dynpgext_magic) && magic->version == 0) {

                // Check if it should be eagerly preloaded
                bool (*eager_preload)() = dlsym(ext_handle, "_Dynpgext_eager_preload");
                bool should_eagerly_preload =
                    (!eager_preload || (eager_preload && eager_preload()));
                bool load = !config->preload || should_eagerly_preload;
                if (load) {
                  // Try loading init
                  void (*action_fn)(const dynpgext_handle *) = dlsym(
                      ext_handle, config->action == LOAD ? "_Dynpgext_init" : "_Dynpgext_fini");
                  if (action_fn != NULL) {
                    // We need to ensure the handle and the updates to global variables (handles,
                    // memory allocation requests, background worker requests) remain after the
                    // temporary context is gone, therefore we switch to the TopMemoryContext
                    MemoryContextSwitchTo(TopMemoryContext);
                    dynpgext_handle *handle = palloc(sizeof(dynpgext_handle));
                    handle->name = pstrdup(control_file.ext_name);
                    handle->version = pstrdup(control_file.ext_version);
                    handle->library_name =
                        get_fitting_library_name(pstrdup(control_file.module_pathname));
                    if (config) {
                      handle->allocate_shmem = config->allocate_shmem;
                      handle->register_bgworker = config->register_bgworker_function;
                    }
                    action_fn(handle);
                    cdeq_handle_push_back(&handles, handle);
                    MemoryContextSwitchTo(memory_context.new);
                  }

                  ereport(
                      LOG,
                      errmsg("Loaded Dynpgext extension %s (version %s)", control_file.ext_name,
                             control_file.ext_version ? control_file.ext_version : "unspecified"));
                }
              }
            }
          }
        }

      } else {
        ereport(WARNING, errmsg("Can't load control file %s", control_basename));
      }
      FreeConfigVariables(head);
    } else {
      int err = errno;
      ereport(WARNING, errmsg("Error opening control file %s", control_basename),
              errdetail("%s", strerror(err)));
    }
  }
}