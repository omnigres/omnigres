#ifndef OMNI_EXT_CONTROL_FILE_H
#define OMNI_EXT_CONTROL_FILE_H

#include <dynpgext.h>

/**
 * @brief Postgres extension control file
 */
typedef struct {
  // Derived from the control file name
  char *ext_name;
  char *ext_version;
  // Derived from the control file content
  char *directory;
  char *default_version;
  char *comment;
  char *encoding;
  char *module_pathname;
  char *requires;

  bool superuser;
  bool trusted;
  bool relocatable;
  char *schema;
} control_file;

struct load_control_file_config {
  /**
   * @brief Are we eagerly preloading at this moment?
   *
   */
  bool preload;
  /**
   * @brief If set, the version that we expect to be loaded
   *
   */
  char *expected_version;
  /**
   * @brief Should we load or unload the extension?
   *
   */
  enum { LOAD, UNLOAD } action;
  dynpgext_allocate_shmem_function allocate_shmem;
  dynpgext_register_bgworker_function register_bgworker_function;
};

void find_control_files(void (*callback)(const char *control_path, void *data), void *data);
void load_control_file(const char *control_path, void *data);

#endif // OMNI_EXT_CONTROL_FILE_H