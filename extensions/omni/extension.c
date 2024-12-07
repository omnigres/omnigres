// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/genam.h>
#include <catalog/indexing.h>
#include <catalog/pg_depend.h>
#include <catalog/pg_extension.h>
#include <commands/extension.h>
#include <executor/executor.h>
#include <miscadmin.h>
#if PG_MAJORVERSION_NUM >= 16
#include <utils/conffiles.h>
#else
#define CONF_FILE_START_DEPTH 0
#endif
#include <utils/fmgroids.h>
#include <utils/lsyscache.h>
#include <utils/rel.h>

#include "omni_common.h"

static char *get_extension_aux_control_filename(const char *name, const char *version) {
  char *result;
  char *scriptdir;

  char sharepath[MAXPGPATH];

  get_share_path(my_exec_path, sharepath);
  scriptdir = (char *)palloc(MAXPGPATH);
  snprintf(scriptdir, MAXPGPATH, "%s/extension", sharepath);

  result = (char *)palloc(MAXPGPATH);
  snprintf(result, MAXPGPATH, "%s/%s--%s.control", scriptdir, name, version);

  pfree(scriptdir);

  return result;
}

static char *get_extension_control_filename(const char *name) {
  char *result;
  char *scriptdir;

  char sharepath[MAXPGPATH];

  get_share_path(my_exec_path, sharepath);
  scriptdir = (char *)palloc(MAXPGPATH);
  snprintf(scriptdir, MAXPGPATH, "%s/extension", sharepath);

  result = (char *)palloc(MAXPGPATH);
  snprintf(result, MAXPGPATH, "%s/%s.control", scriptdir, name);

  pfree(scriptdir);

  return result;
}

MODULE_FUNCTION char *get_extension_module_pathname(const char *name, const char *version) {
  char *filename;
  char *result = NULL;

  filename = get_extension_aux_control_filename(name, version);
  FILE *file;

try_read:
  if ((file = AllocateFile(filename, "r")) == NULL) {
    if (errno == ENOENT) {
      /* no complaint for missing auxiliary file */
      if (version) {
        pfree(filename);
        filename = get_extension_control_filename(name);
        version = NULL;
        goto try_read;
      }
    }
    ereport(ERROR, (errcode_for_file_access(),
                    errmsg("could not open extension control file \"%s\": %m", filename)));
  }
  ConfigVariable *item, *head = NULL, *tail = NULL;

  (void)ParseConfigFp(file, filename, CONF_FILE_START_DEPTH, ERROR, &head, &tail);

  for (item = head; item != NULL; item = item->next) {
    if (strcmp(item->name, "module_pathname") == 0) {
      result = pstrdup(item->value);
    }
  }

  FreeFile(file);
  return result;
}

static char *get_extension_version(char *extname, bool missing_ok) {
  char *result;

  Relation rel = table_open(ExtensionRelationId, AccessShareLock);

  ScanKeyData entry[1];
  ScanKeyInit(&entry[0], Anum_pg_extension_extname, BTEqualStrategyNumber, F_NAMEEQ,
              CStringGetDatum(extname));

  SysScanDesc scandesc = systable_beginscan(rel, ExtensionNameIndexId, true, NULL, 1, entry);

  HeapTuple tuple = systable_getnext(scandesc);

  if (HeapTupleIsValid(tuple)) {
    bool isnull = false;
    Datum version_datum = heap_getattr(tuple, Anum_pg_extension_extversion, rel->rd_att, &isnull);
    result = isnull ? NULL : text_to_cstring(DatumGetTextPP(version_datum));
  } else {
    result = NULL;
  }

  systable_endscan(scandesc);

  table_close(rel, AccessShareLock);

  if (result == NULL && !missing_ok) {
    ereport(ERROR, (errcode(ERRCODE_UNDEFINED_OBJECT),
                    errmsg("extension \"%s\" does not exist", extname)));
  }

  return result;
}

// In the event of a rollback, ensure we review the list of extensions
// as we created an extension, and it may have created hooks and functions
// we will no longer have after the rollback
static void force_backend_reload_on_rollback(XactEvent event, void *arg) {
  if (event == XACT_EVENT_ABORT) {
    backend_force_reload = true;
  }
}

MODULE_FUNCTION void extension_upgrade_hook(omni_hook_handle *handle, PlannedStmt *pstmt,
                                            const char *queryString, bool readOnlyTree,
                                            ProcessUtilityContext context, ParamListInfo params,
                                            QueryEnvironment *queryEnv, DestReceiver *dest,
                                            QueryCompletion *qc) {
  struct ExtensionUpgradeHookState {
    NodeTag nodeTag;
    char *extname;
    char *module_pathname;
  };
  struct ExtensionUpgradeHookState *state = (struct ExtensionUpgradeHookState *)handle->ctx;
  bool IsFirstInvocation = state == NULL;
  MemoryContext memcxt = TopTransactionContext;
  if (state == NULL) {
    handle->ctx = state =
        (struct ExtensionUpgradeHookState *)MemoryContextAllocZero(memcxt, sizeof(*state));
    Assert(state->extname == NULL);
    Assert(state->module_pathname == NULL);
    state->nodeTag = nodeTag(pstmt->utilityStmt);
  }

  switch (state->nodeTag) {
  case T_CreateExtensionStmt:
  case T_AlterExtensionStmt:
    backend_force_reload = true;
    break;
  case T_DropStmt:
    if (IsFirstInvocation &&
        castNode(DropStmt, pstmt->utilityStmt)->removeType == OBJECT_EXTENSION) {
      backend_force_reload = true;
    }
  default:
    break;
  }
  // Once done, set up a callback that ensures we force cleanup
  // upon rollback.
  if (backend_force_reload && !IsFirstInvocation) {
    MemoryContext oldcontext = MemoryContextSwitchTo(TopTransactionContext);
    struct xact_oneshot_callback *cb = palloc(sizeof(*cb));
    cb->fn = force_backend_reload_on_rollback;
    cb->arg = cb;
    xact_oneshot_callbacks = list_append_unique_ptr(xact_oneshot_callbacks, cb);
    MemoryContextSwitchTo(oldcontext);
  }

  if (state->nodeTag == T_AlterExtensionStmt) {
    static Oid extoid = InvalidOid;
    if (IsFirstInvocation) {
      char *extname =
          MemoryContextStrdup(memcxt, castNode(AlterExtensionStmt, pstmt->utilityStmt)->extname);
      extoid = get_extension_oid(extname, true);
      // Indicate that we're past the first pass with the context
      char *extver = get_extension_version(extname, true);
      char *module_pathname =
          MemoryContextStrdup(memcxt, get_extension_module_pathname(extname, extver));
      state->extname = extname;
      state->module_pathname = module_pathname;
    } else {
      char *old_module_pathname = state->module_pathname;
      // Second pass

      // Get necessary extension information
      char *extver = get_extension_version(state->extname, true);
      char *module_pathname = get_extension_module_pathname(state->extname, extver);

      // Obtain a lock on pg_proc
      Relation proc_rel = table_open(ProcedureRelationId, RowExclusiveLock);

      // Go through pg_depend for for functions defined by this extensions
      Relation depend_rel = table_open(DependRelationId, AccessShareLock);

      // Scan refclassid = pg_extension and refobjid = extoid
      ScanKeyData key[2];
      ScanKeyInit(&key[0], Anum_pg_depend_refclassid, BTEqualStrategyNumber, F_OIDEQ,
                  ObjectIdGetDatum(ExtensionRelationId));
      ScanKeyInit(&key[1], Anum_pg_depend_refobjid, BTEqualStrategyNumber, F_OIDEQ,
                  ObjectIdGetDatum(extoid));

      SysScanDesc dep_scan =
          systable_beginscan(depend_rel, DependReferenceIndexId, true, NULL, 2, key);

      HeapTuple dep_tup;
      bool updated = false;

      while (HeapTupleIsValid(dep_tup = systable_getnext(dep_scan))) {
        Form_pg_depend pg_depend = (Form_pg_depend)GETSTRUCT(dep_tup);

        // Ensure the dependent object is a procedure
        if (pg_depend->classid == ProcedureRelationId) {
          HeapTuple tup = SearchSysCache1(PROCOID, ObjectIdGetDatum(pg_depend->objid));
          if (HeapTupleIsValid(tup)) {
            Form_pg_proc oldproc = (Form_pg_proc)GETSTRUCT(tup);
            // Ensure the procedure is implemented in C
            if (oldproc->prolang == ClanguageId) {
              bool nulls[Natts_pg_proc];
              Datum values[Natts_pg_proc];
              bool replaces[Natts_pg_proc];
              for (int i = 0; i < Natts_pg_proc; ++i) {
                nulls[i] = false;
                values[i] = (Datum)0;
                replaces[i] = false;
              }
              // Ensure to replace only those procedures that had a matching module_pathname
              // from the previous version of the extension (we recorded that in the context)
              bool isnull;
              Datum probin =
                  heap_getattr(tup, Anum_pg_proc_probin, RelationGetDescr(proc_rel), &isnull);
              if (!isnull && strcmp(old_module_pathname, TextDatumGetCString(probin)) == 0) {
                // Get the new module_pathname in
                values[Anum_pg_proc_probin - 1] = CStringGetTextDatum(module_pathname);
                replaces[Anum_pg_proc_probin - 1] = true;

                HeapTuple newtup =
                    heap_modify_tuple(tup, RelationGetDescr(proc_rel), values, nulls, replaces);

                // Update the record
                CatalogTupleUpdate(proc_rel, &newtup->t_self, newtup);
                updated = true;
              }
            }
            ReleaseSysCache(tup);
          }
        }
      }

      if (updated) {
        // It is important to increment the counter here so that the changes
        // we've made are visible to our pg_proc scanner
        CommandCounterIncrement();
      }

      systable_endscan(dep_scan);

      table_close(depend_rel, AccessShareLock);
      table_close(proc_rel, RowExclusiveLock);
    }
  }
  if (!IsFirstInvocation) {
    handle->ctx = NULL;
  }
}
