/**
 * @file funsig_type.c
 *
 */

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/heapam.h>
#include <access/htup_details.h>
#include <access/table.h>
#include <catalog/namespace.h>
#include <catalog/pg_cast.h>
#include <catalog/pg_collation.h>
#include <catalog/pg_language.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>
#include <commands/typecmds.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <utils/array.h>
#include <utils/builtins.h>
#include <utils/fmgroids.h>
#include <utils/lsyscache.h>
#include <utils/regproc.h>
#include <utils/syscache.h>

#include "sum_type.h"

static Datum function_signature_search(PG_FUNCTION_ARGS, char *input, bool missing_as_null) {

  Node *escontext = fcinfo->context;

  // Get the type that we need to return
  Oid typeoid = get_func_rettype(fcinfo->flinfo->fn_oid);

  // Find matching record in our table
  Oid types = get_relname_relid("function_signature_types", get_namespace_oid("omni_types", false));
  Relation rel = table_open(types, AccessShareLock);
  TupleDesc tupdesc = RelationGetDescr(rel);
  TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
  Oid matching_function = InvalidOid;
  for (;;) {
    HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
    // The end
    if (tup == NULL)
      break;

    bool isnull;
    // Matching type
    // TODO: search by index
    if (typeoid == DatumGetObjectId(heap_getattr(tup, 1, tupdesc, &isnull))) {

      AnyArrayType *arguments = DatumGetAnyArrayP(heap_getattr(tup, 2, tupdesc, &isnull));

      // If argument lengths are not matching, move on
      int nargs = ArrayGetNItems(AARR_NDIM(arguments), AARR_DIMS(arguments));

      List *names = stringToQualifiedNameList(input
#if PG_MAJORVERSION_NUM > 15
                                              ,
                                              escontext
#endif
      );
#if PG_MAJORVERSION_NUM > 18
      int fgc_flags;
#endif

      FuncCandidateList candidates = FuncnameGetCandidates(names, nargs, NIL, false, false,
#if PG_MAJORVERSION_NUM > 13
                                                           false,
#endif
                                                           true
#if PG_MAJORVERSION_NUM > 18
                                                           ,
                                                           &fgc_flags
#endif
      );

      while (candidates) {
        // Iterate through arguments
        ArrayIterator it = array_create_iterator((ArrayType *)arguments, 0, NULL);

        Datum elem;
        // Iterate through argument types
        int i = 0;
        while (array_iterate(it, &elem, &isnull)) {
          if (isnull) {
            continue;
          }
          if (DatumGetObjectId(elem) != candidates->args[i]) {
            goto done_iter;
          }
          i++;
        }
        // Get return types of the looked up function
        HeapTuple proc_tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(candidates->oid));
        Assert(HeapTupleIsValid(proc_tuple));
        Form_pg_proc proc_struct = (Form_pg_proc)GETSTRUCT(proc_tuple);
        Oid function_ret_type = proc_struct->prorettype;
        ReleaseSysCache(proc_tuple);

        if (function_ret_type != DatumGetObjectId(heap_getattr(tup, 3, tupdesc, &isnull))) {
          goto done_iter;
        }

        matching_function = candidates->oid;
      done_iter:
        array_free_iterator(it);
        if (OidIsValid(matching_function)) {
          break;
        }
        candidates = candidates->next;
      }
    }
  }
  if (scan->rs_rd->rd_tableam->scan_end) {
    scan->rs_rd->rd_tableam->scan_end(scan);
  }
  table_close(rel, AccessShareLock);
  if (!OidIsValid(matching_function)) {
    MemoryContext oldcontext = CurrentMemoryContext;
    PG_TRY();
    { DirectFunctionCall1(regprocin, CStringGetDatum(input)); }
    PG_CATCH();
    {
      int errcode = geterrcode();
      MemoryContextSwitchTo(oldcontext);
      if (!missing_as_null && errcode != ERRCODE_AMBIGUOUS_FUNCTION) {
        PG_RE_THROW();
      } else {
        FlushErrorState();
      }
    }
    PG_END_TRY();
    if (!missing_as_null) {
      ereport(ERROR, errmsg("function \"%s\" does not match the signature of the type", input));
    } else {
      PG_RETURN_NULL();
    }
  }
  PG_RETURN_OID(matching_function);
}

PG_FUNCTION_INFO_V1(function_signature_in);

Datum function_signature_in(PG_FUNCTION_ARGS) {
  char *input = PG_GETARG_CSTRING(0);
  return function_signature_search(fcinfo, input, false);
}

PG_FUNCTION_INFO_V1(conforming_function);

Datum conforming_function(PG_FUNCTION_ARGS) {
  char *input = text_to_cstring(PG_GETARG_TEXT_PP(0));
  return function_signature_search(fcinfo, input, true);
}

PG_FUNCTION_INFO_V1(invoke);

Datum invoke(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("Can't invoke a NULL function"));
  }

  FmgrInfo flinfo;
  MemoryContext oldcontext = CurrentMemoryContext;
  PG_TRY();
  { fmgr_info(PG_GETARG_OID(0), &flinfo); }
  PG_CATCH();
  {
    MemoryContextSwitchTo(oldcontext);
    FlushErrorState();
    ereport(ERROR, errmsg("function does not exist"));
  }
  PG_END_TRY();

  LOCAL_FCINFO(fcinfo1, FUNC_MAX_ARGS); // FIXME: space-inefficient

  InitFunctionCallInfoData(*fcinfo1, &flinfo, fcinfo->nargs - 1, fcinfo->fncollation,
                           fcinfo->context, fcinfo->resultinfo);

  for (short i = 0; i < fcinfo->nargs - 1; i++) {
    fcinfo1->args[i] = fcinfo->args[i + 1];
    if (fcinfo1->flinfo->fn_strict && fcinfo1->args[i].isnull) {
      PG_RETURN_NULL();
    }
  }

  return FunctionCallInvoke(fcinfo1);
}
