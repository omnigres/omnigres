#ifndef OMNI_EXT_DB_H
#define OMNI_EXT_DB_H

#include <libgluepg_stc.h>

typedef struct {
  char *extname;
  char *extversion;
} extension_key;

int cmap_extensions_key_cmp(const extension_key *left, const extension_key *right);

#define i_tag extensions
#define i_key extension_key
#define i_val Oid
#define i_eq cmap_extensions_key_cmp
#include <stc/cmap.h>

cmap_extensions created_extensions();

#endif // OMNI_EXT_DB_H
