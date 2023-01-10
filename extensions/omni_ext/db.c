#include <string.h>

// clang-format off
#include <postgres.h>
#include <postmaster/bgworker.h>
// clang-format on
#define CATALOG_VARLEN

#include <access/heapam.h>
#include <access/sdir.h>
#include <access/table.h>
#include <access/tableam.h>
#include <access/xact.h>
#include <catalog/pg_database.h>
#include <catalog/pg_extension.h>
#include <utils/builtins.h>

#include "db.h"

int cmap_extensions_key_cmp(const extension_key *left, const extension_key *right) {
  int name_cmp = strcmp(left->extname, right->extname);
  if (name_cmp != 0) {
    return name_cmp;
  }
  return strcmp(left->extversion, right->extversion);
}

cmap_extensions created_extensions() {

  cmap_extensions extensions = cmap_extensions_init();

  Relation rel = table_open(ExtensionRelationId, AccessShareLock);
  TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
  for (;;) {
    HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
    if (tup == NULL)
      break;
    Form_pg_extension ext = (Form_pg_extension)GETSTRUCT(tup);
    bool is_version_null;
    Datum version_datum = heap_getattr(tup, 6, rel->rd_att, &is_version_null);
    text *version = is_version_null ? NULL : DatumGetTextPP(version_datum);
    extension_key key = {.extname = pstrdup(ext->extname.data),
                         .extversion = is_version_null ? "" : text_to_cstring(version)};
    cmap_extensions_insert(&extensions, key, ext->oid);
  }
  if (scan->rs_rd->rd_tableam->scan_end) {
    scan->rs_rd->rd_tableam->scan_end(scan);
  }
  table_close(rel, AccessShareLock);

  return extensions;
}