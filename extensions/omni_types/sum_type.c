/**
 * @file sum_type.c
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
#include <catalog/pg_operator.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>
#include <commands/defrem.h>
#include <commands/typecmds.h>
#include <executor/spi.h>
#include <miscadmin.h>
#include <utils/array.h>
#include <utils/builtins.h>
#include <utils/fmgroids.h>
#include <utils/lsyscache.h>
#include <utils/snapmgr.h>
#include <utils/syscache.h>

#include "sum_type.h"

#include <parser/parse_func.h>
#include <tcop/pquery.h>

/**
 * Returns sum value's variant
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_variant);

/**
 * Defines a sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_type);

/**
 * Adds a variant to the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(add_variant);

/**
 * Converts cstring into a sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_in);
/**
 * Converts sum type to a cstring
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_out);
/**
 * Converts bytea into a sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_recv);
/**
 * Converts sum type to a bytea
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_send);
/**
 * Casts sum type to a variant
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_cast_to);
/**
 * Casts variant to sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_cast_from);

/**
 * Equality comparison of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_eq);

/**
 * Non-equality comparison of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_neq);

/**
 * Comparison of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_lt);

/**
 * Comparison of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_lte);

/**
 * Comparison of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_gt);

/**
 * Comparison of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_gte);

/**
 * Comparison of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_cmp);

/**
 * Hash of the sum type
 * @param fcinfo
 * @return
 */
PG_FUNCTION_INFO_V1(sum_hash);

/**
 * Extract variant value from a sum type.
 *
 * If invalid, `variant` will contain `InvalidOid`.
 *
 * @param arg sum type value
 * @param sum_type_oid sum type
 * @param variant returns the variant Oid
 * @param val returns the extracted variant
 */
static void get_variant_val(Datum arg, Oid sum_type_oid, Oid *variant, Datum *val,
                            Discriminant *discriminant);
typedef struct SumTypeCacheEntry {
  Oid sum_type_oid;
  uint32 hash;
  uint32 status;
  TransactionId xmin;
  oidvector *variant_type_oids;
  oidvector *variant_eq_ops;
  oidvector *variant_opc_hash;
  oidvector *variant_opc_btree;
} SumTypeCacheEntry;

static SumTypeCacheEntry *lookup_sumtype_cache(Datum sum_type_oid);

static Oid get_fn_expr_argtype_reliable(FmgrInfo *info, int argnum) {
  if (info == NULL || info->fn_nargs < argnum + 1) {
    return InvalidOid;
  }
  Oid type = get_fn_expr_argtype(info, argnum);
  if (!OidIsValid(type)) {
    HeapTuple tup = SearchSysCache1(PROCOID, info->fn_oid);
    if (HeapTupleIsValid(tup)) {
      type = ((Form_pg_proc)GETSTRUCT(tup))->proargtypes.values[argnum];
      ReleaseSysCache(tup);
    }
  }
  return type;
}

Datum sum_variant(PG_FUNCTION_ARGS) {

  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("Sum type value can't be null"));
  }

  Oid sum_type_oid = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg = PG_GETARG_DATUM(0);

  Oid variant;
  get_variant_val(arg, sum_type_oid, &variant, NULL, NULL);

  if (variant == InvalidOid) {
    PG_RETURN_NULL();
  }

  PG_RETURN_OID(variant);
}

/**
 * Makes a sum type from a given variant
 * @param sum_type_len length of the sum type
 * @param discriminant discriminant
 * @param variant_type_len length of the variant type
 * @param variant_byval is variant passed by value?
 * @param variant_value the actual variant
 * @return sum type
 */
static Datum make_variant(int16 sum_type_len, Discriminant discriminant, int16 variant_type_len,
                          bool variant_byval, Datum variant_value) {
  // If sum type is fixed size
  if (sum_type_len != -1) {
    FixedSizeVariant *fixed_size = palloc(sum_type_len);
    fixed_size->discriminant = discriminant;

    // If the variant is passed by val
    if (variant_byval) {
      // Copy the variant itself
      memcpy(&fixed_size->data, &variant_value, variant_type_len);
    } else {
      // Copy the variant from behind the pointer
      memcpy(&fixed_size->data, DatumGetPointer(variant_value), variant_type_len);
    }
    PG_RETURN_POINTER(fixed_size);
  } else {
    // If sum type is variable size,
    struct varlena *variant_varlena =
        variant_type_len == -1 ? PG_DETOAST_DATUM_PACKED(variant_value) : NULL;
    size_t sz = variant_type_len == -1 ? VARSIZE_ANY(variant_varlena) : variant_type_len;
    struct varlena *varsize = palloc(VARHDRSZ + sizeof(VarSizeVariant) + sz);
    SET_VARSIZE(varsize, VARHDRSZ + sizeof(VarSizeVariant) + sz);
    VarSizeVariant *var_size_variant = (VarSizeVariant *)VARDATA_ANY(varsize);
    var_size_variant->discriminant = discriminant;
    SET_VARSIZE(&var_size_variant->data, sz);
    // If the variant is passed by val
    if (variant_byval) {
      // Copy the variant itself
      memcpy(VARDATA_ANY(&var_size_variant->data), &variant_value, sz);
    } else {
      // Copy the variant from behind the pointer
      // If the variant itself is a variable size, it'll copy the `varlena`
      // which is intentional
      memcpy(&var_size_variant->data,
             variant_varlena ? (void *)variant_varlena : DatumGetPointer(variant_value), sz);
    }
    PG_RETURN_POINTER(varsize);
  }
}

Datum sum_in(PG_FUNCTION_ARGS) {

  char *input = PG_GETARG_CSTRING(0);
  if (input[strlen(input) - 1] != ')') {
    // No closing paren
    ereport(ERROR, errmsg("Invalid syntax"), errdetail("missing trailing parenthesis"));
  }

  HeapTuple proc_tuple = SearchSysCache1(PROCOID, fcinfo->flinfo->fn_oid);
  Assert(HeapTupleIsValid(proc_tuple));
  Form_pg_proc proc_struct = (Form_pg_proc)GETSTRUCT(proc_tuple);
  Oid sum_type_oid = proc_struct->prorettype;
  ReleaseSysCache(proc_tuple);

  HeapTuple sum_type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(sum_type_oid));
  Assert(HeapTupleIsValid(sum_type_tup));
  Form_pg_type sum_typtup = (Form_pg_type)GETSTRUCT(sum_type_tup);
  int16 sum_type_len = sum_typtup->typlen;
  ReleaseSysCache(sum_type_tup);

  // Find matching variant
  Discriminant discriminant = 0;
  Oid variant = InvalidOid;
  SumTypeCacheEntry *entry = lookup_sumtype_cache(sum_type_oid);
  if (entry) {
    for (int i = 0; i < entry->variant_type_oids->dim1; i++) {
      char *type = format_type_be(entry->variant_type_oids->values[i]);
      if (strncmp(input, type, strlen(type)) == 0 && input[strlen(type)] == '(') {
        variant = entry->variant_type_oids->values[i];
        discriminant = i;
        break;
      }
    }
  }

  HeapTuple type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(variant));
  Assert(HeapTupleIsValid(type_tup));
  Form_pg_type typtup = (Form_pg_type)GETSTRUCT(type_tup);
  int16 variant_type_len = typtup->typlen;
  bool variant_byval = typtup->typbyval;
  regproc variant_input_function_oid = typtup->typinput;
  int32 variant_typmod = typtup->typmodin;
  Oid variant_ioparam = OidIsValid(typtup->typelem) ? typtup->typelem : typtup->oid;
  ReleaseSysCache(type_tup);

  char *left_paren = strchr(input, '(');
  unsigned long effective_input_size = strlen(input) - (left_paren - input) - 1;
  char *modified_input = (char *)palloc(effective_input_size);
  strncpy(modified_input, left_paren + 1, effective_input_size - 1);
  modified_input[effective_input_size - 1] = 0;

  Datum result = OidInputFunctionCall(variant_input_function_oid, modified_input, variant_ioparam,
                                      variant_typmod);

  return make_variant(sum_type_len, discriminant, variant_type_len, variant_byval, result);
}

Datum sum_out(PG_FUNCTION_ARGS) {
  HeapTuple proc_tuple = SearchSysCache1(PROCOID, fcinfo->flinfo->fn_oid);
  Assert(HeapTupleIsValid(proc_tuple));
  Form_pg_proc proc_struct = (Form_pg_proc)GETSTRUCT(proc_tuple);
  Oid sum_type_oid = proc_struct->proargtypes.values[0];
  ReleaseSysCache(proc_tuple);

  Datum arg = PG_GETARG_DATUM(0);

  Oid variant;
  Datum val;
  get_variant_val(arg, sum_type_oid, &variant, &val, NULL);

  HeapTuple type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(variant));
  Assert(HeapTupleIsValid(type_tup));
  Form_pg_type typtup = (Form_pg_type)GETSTRUCT(type_tup);
  regproc variant_output_function_oid = typtup->typoutput;
  char *variant_name = format_type_be(variant);

  char *out = OidOutputFunctionCall(variant_output_function_oid, val);

  StringInfoData string;
  initStringInfo(&string);
  appendStringInfo(&string, "%s(%s)", variant_name, out);

  ReleaseSysCache(type_tup);

  PG_RETURN_CSTRING(string.data);
}

Datum sum_recv(PG_FUNCTION_ARGS) {

  bytea *input = PG_GETARG_BYTEA_PP(0);
  if (VARSIZE_ANY_EXHDR(input) < sizeof(Discriminant)) {
    ereport(ERROR, errmsg("input is too short to fit a discriminant"));
  }

  HeapTuple proc_tuple = SearchSysCache1(PROCOID, fcinfo->flinfo->fn_oid);
  Assert(HeapTupleIsValid(proc_tuple));
  Form_pg_proc proc_struct = (Form_pg_proc)GETSTRUCT(proc_tuple);
  Oid sum_type_oid = proc_struct->prorettype;
  ReleaseSysCache(proc_tuple);

  HeapTuple sum_type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(sum_type_oid));
  Assert(HeapTupleIsValid(sum_type_tup));
  Form_pg_type sum_typtup = (Form_pg_type)GETSTRUCT(sum_type_tup);
  int16 sum_type_len = sum_typtup->typlen;
  ReleaseSysCache(sum_type_tup);

  // Find matching variant
  Discriminant discriminant = (Discriminant)pg_ntoh32(*((Discriminant *)VARDATA_ANY(input)));
  ereport(NOTICE, errmsg("discriminant %d", discriminant));
  StaticAssertStmt(sizeof(Discriminant) == 4, "must be 32-bit");
  Oid variant = InvalidOid;
  SumTypeCacheEntry *entry = lookup_sumtype_cache(sum_type_oid);
  if (entry) {
    if (entry->variant_type_oids->dim1 < discriminant + 1) {
      ereport(ERROR, errmsg("invalid discriminant"),
              errdetail("expected 0..%d, got %d", entry->variant_type_oids->ndim - 1,
                        (int)discriminant));
    }
    variant = entry->variant_type_oids->values[discriminant];
  }

  if (variant == InvalidOid) {
    ereport(ERROR, errmsg("No valid variant found"));
  }

  HeapTuple type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(variant));
  Assert(HeapTupleIsValid(type_tup));
  Form_pg_type typtup = (Form_pg_type)GETSTRUCT(type_tup);
  int16 variant_type_len = typtup->typlen;
  bool variant_byval = typtup->typbyval;
  regproc variant_recv_function_oid = typtup->typreceive;
  int32 variant_typmod = typtup->typmodin;
  Oid variant_ioparam = OidIsValid(typtup->typelem) ? typtup->typelem : typtup->oid;
  ReleaseSysCache(type_tup);

  StringInfoData string;
  initStringInfo(&string);
  string.data = (char *)(((Discriminant *)VARDATA_ANY(input)) + 1);
  string.len = VARSIZE_ANY_EXHDR(input) - sizeof(Discriminant);
  Datum result =
      OidReceiveFunctionCall(variant_recv_function_oid, &string, variant_ioparam, variant_typmod);

  return make_variant(sum_type_len, discriminant, variant_type_len, variant_byval, result);
}

Datum sum_send(PG_FUNCTION_ARGS) {
  HeapTuple proc_tuple = SearchSysCache1(PROCOID, fcinfo->flinfo->fn_oid);
  Assert(HeapTupleIsValid(proc_tuple));
  Form_pg_proc proc_struct = (Form_pg_proc)GETSTRUCT(proc_tuple);
  Oid sum_type_oid = proc_struct->proargtypes.values[0];
  ReleaseSysCache(proc_tuple);

  Datum arg = PG_GETARG_DATUM(0);

  Oid variant;
  Datum val;
  Discriminant discriminant;
  get_variant_val(arg, sum_type_oid, &variant, &val, &discriminant);

  HeapTuple type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(variant));
  Assert(HeapTupleIsValid(type_tup));
  Form_pg_type typtup = (Form_pg_type)GETSTRUCT(type_tup);
  regproc variant_send_function_oid = typtup->typsend;
  ReleaseSysCache(type_tup);

  struct varlena *out = OidSendFunctionCall(variant_send_function_oid, val);

  size_t len = sizeof(Discriminant) + VARSIZE_ANY(out);
  struct varlena *bytes = palloc(len);
  SET_VARSIZE(bytes, len);
  StaticAssertStmt(sizeof(Discriminant) == 4, "must be 32-bit");
  *((Discriminant *)VARDATA_ANY(bytes)) = pg_hton32(discriminant);

  memcpy(VARDATA_ANY(bytes) + sizeof(Discriminant), VARDATA_ANY(out), VARSIZE_ANY_EXHDR(out));

  PG_RETURN_BYTEA_P(bytes);
}

Datum sum_cast_to(PG_FUNCTION_ARGS) {
  HeapTuple proc_tuple = SearchSysCache1(PROCOID, fcinfo->flinfo->fn_oid);
  Assert(HeapTupleIsValid(proc_tuple));
  Form_pg_proc proc_struct = (Form_pg_proc)GETSTRUCT(proc_tuple);
  Oid sum_type_oid = proc_struct->proargtypes.values[0];
  Oid variant_type_oid = proc_struct->prorettype;
  ReleaseSysCache(proc_tuple);

  Datum arg = PG_GETARG_DATUM(0);

  Oid variant;
  Datum val;
  get_variant_val(arg, sum_type_oid, &variant, &val, NULL);

  if (variant != variant_type_oid) {
    PG_RETURN_NULL();
  }

  return val;
}

Datum sum_cast_from(PG_FUNCTION_ARGS) {
  HeapTuple proc_tuple = SearchSysCache1(PROCOID, fcinfo->flinfo->fn_oid);
  Assert(HeapTupleIsValid(proc_tuple));
  Form_pg_proc proc_struct = (Form_pg_proc)GETSTRUCT(proc_tuple);
  Oid sum_type_oid = proc_struct->prorettype;
  Oid variant_type_oid = proc_struct->proargtypes.values[0];
  ReleaseSysCache(proc_tuple);

  // Find matching variant
  bool discriminant_found = false;
  Discriminant discriminant = 0;

  SumTypeCacheEntry *entry = lookup_sumtype_cache(sum_type_oid);
  if (entry) {
    for (int i = 0; i < entry->variant_type_oids->dim1; i++) {
      if (entry->variant_type_oids->values[i] == variant_type_oid) {
        discriminant = i;
        discriminant_found = true;
        break;
      }
    }
  }

  if (!discriminant_found) {
    ereport(ERROR, errmsg("No valid variant found"));
  }

  HeapTuple sum_type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(sum_type_oid));
  Assert(HeapTupleIsValid(sum_type_tup));
  Form_pg_type sum_typtup = (Form_pg_type)GETSTRUCT(sum_type_tup);
  int16 sum_type_len = sum_typtup->typlen;
  ReleaseSysCache(sum_type_tup);

  HeapTuple type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(variant_type_oid));
  Assert(HeapTupleIsValid(type_tup));
  Form_pg_type typtup = (Form_pg_type)GETSTRUCT(type_tup);
  int16 variant_type_len = typtup->typlen;
  bool variant_byval = typtup->typbyval;
  ReleaseSysCache(type_tup);

  return make_variant(sum_type_len, discriminant, variant_type_len, variant_byval,
                      PG_GETARG_DATUM(0));
}

struct sumtypehash_hash;
static struct sumtypehash_hash *sumtypehash = NULL;

#define SH_PREFIX sumtypehash
#define SH_ELEMENT_TYPE SumTypeCacheEntry
#define SH_KEY_TYPE Oid
#define SH_KEY sum_type_oid
#define SH_HASH_KEY(tb, key) key
#define SH_EQUAL(tb, a, b) a == b
#define SH_SCOPE static
#define SH_STORE_HASH
#define SH_GET_HASH(tb, a) a->hash
#define SH_DECLARE
#define SH_DEFINE
#include <lib/simplehash.h>

// The function below is extracted from Postgres source code

/* oper_select_candidate()
 *      Given the input argtype array and one or more candidates
 *      for the operator, attempt to resolve the conflict.
 *
 * Returns FUNCDETAIL_NOTFOUND, FUNCDETAIL_MULTIPLE, or FUNCDETAIL_NORMAL.
 * In the success case the Oid of the best candidate is stored in *operOid.
 *
 * Note that the caller has already determined that there is no candidate
 * exactly matching the input argtype(s).  Incompatible candidates are not yet
 * pruned away, however.
 */
static FuncDetailCode oper_select_candidate(int nargs, Oid *input_typeids,
                                            FuncCandidateList candidates,
                                            Oid *operOid) /* output argument */
{
#if PG_MAJORVERSION_NUM > 18
#error "check if this version of the function is still good"
#endif
  int ncandidates;

  /*
   * Delete any candidates that cannot actually accept the given input
   * types, whether directly or by coercion.
   */
  ncandidates = func_match_argtypes(nargs, input_typeids, candidates, &candidates);

  /* Done if no candidate or only one candidate survives */
  if (ncandidates == 0) {
    *operOid = InvalidOid;
    return FUNCDETAIL_NOTFOUND;
  }
  if (ncandidates == 1) {
    *operOid = candidates->oid;
    return FUNCDETAIL_NORMAL;
  }

  /*
   * Use the same heuristics as for ambiguous functions to resolve the
   * conflict.
   */
  candidates = func_select_candidate(nargs, input_typeids, candidates);

  if (candidates) {
    *operOid = candidates->oid;
    return FUNCDETAIL_NORMAL;
  }

  *operOid = InvalidOid;
  return FUNCDETAIL_MULTIPLE; /* failed to select a best candidate */
}

static Oid lookup_operator(Oid type, char *name) {
  List *op = list_make1(makeString(name));
  Oid oid = OpernameGetOprid(op, type, type);
  if (!OidIsValid(oid)) {
    FuncCandidateList clist = OpernameGetCandidates(op, 'b', false);

    if (clist != NULL) {
      oper_select_candidate(2, (Oid[2]){type, type}, clist, &oid);
    }
  }
  return oid;
}

static SumTypeCacheEntry *lookup_sumtype_cache(Datum sum_type_oid) {
  bool cache_entry_found;
  bool matching_entry_found = false;
  SumTypeCacheEntry *entry = NULL;

  if (sumtypehash == NULL) {
    sumtypehash = sumtypehash_create(TopMemoryContext, 1024, NULL);
  }

  // If there's no snapshot, assume it to be zero so that when one appears,
  // we'll purge this entry
  TransactionId xmin = ActiveSnapshotSet() ? GetActiveSnapshot()->xmin : 0;
  while (entry == NULL || entry->xmin != xmin) {
    entry = sumtypehash_insert(sumtypehash, sum_type_oid, &cache_entry_found);
    if (!cache_entry_found) {
      entry->xmin = xmin;

      Oid types = get_relname_relid("sum_types", get_namespace_oid("omni_types", false));

      Relation rel = table_open(types, AccessShareLock);
      TupleDesc tupdesc = RelationGetDescr(rel);
      TableScanDesc scan = table_beginscan_catalog(rel, 0, NULL);
      for (;;) {
        HeapTuple tup = heap_getnext(scan, ForwardScanDirection);
        // The end
        if (tup == NULL)
          break;

        bool isnull;
        // Matching type
        if (sum_type_oid == DatumGetObjectId(heap_getattr(tup, 1, tupdesc, &isnull))) {
          Datum variants = heap_getattr(tup, 2, tupdesc, &isnull);
          ArrayType *arr = DatumGetArrayTypeP(variants);
          Assert(ARR_NDIM(arr) == 1);
          int n = ARR_DIMS(arr)[0];

          Datum *elems;
          bool *nulls;
          deconstruct_array(arr, REGTYPEOID, sizeof(Oid), true, TYPALIGN_INT, &elems, &nulls, &n);

          entry->variant_type_oids = (oidvector *)MemoryContextAlloc(
              TopMemoryContext, n * sizeof(Oid) + sizeof(oidvector));
          SET_VARSIZE(entry->variant_type_oids, (n * sizeof(Oid) + sizeof(oidvector)));
          entry->variant_type_oids->ndim = 1;
          entry->variant_type_oids->dataoffset = 0; /* never any nulls */
          entry->variant_type_oids->elemtype = OIDOID;
          entry->variant_type_oids->dim1 = n;
          entry->variant_type_oids->lbound1 = 0;

          entry->variant_eq_ops = (oidvector *)MemoryContextAlloc(
              TopMemoryContext, n * sizeof(Oid) + sizeof(oidvector));
          SET_VARSIZE(entry->variant_eq_ops, (n * sizeof(Oid) + sizeof(oidvector)));
          entry->variant_eq_ops->ndim = 1;
          entry->variant_eq_ops->dataoffset = 0; /* never any nulls */
          entry->variant_eq_ops->elemtype = OIDOID;
          entry->variant_eq_ops->dim1 = n;
          entry->variant_eq_ops->lbound1 = 0;

          entry->variant_opc_btree = (oidvector *)MemoryContextAlloc(
              TopMemoryContext, n * sizeof(Oid) + sizeof(oidvector));
          SET_VARSIZE(entry->variant_opc_btree, (n * sizeof(Oid) + sizeof(oidvector)));
          entry->variant_opc_btree->ndim = 1;
          entry->variant_opc_btree->dataoffset = 0; /* never any nulls */
          entry->variant_opc_btree->elemtype = OIDOID;
          entry->variant_opc_btree->dim1 = n;
          entry->variant_opc_btree->lbound1 = 0;

          entry->variant_opc_hash = (oidvector *)MemoryContextAlloc(
              TopMemoryContext, n * sizeof(Oid) + sizeof(oidvector));
          SET_VARSIZE(entry->variant_opc_hash, (n * sizeof(Oid) + sizeof(oidvector)));
          entry->variant_opc_hash->ndim = 1;
          entry->variant_opc_hash->dataoffset = 0; /* never any nulls */
          entry->variant_opc_hash->elemtype = OIDOID;
          entry->variant_opc_hash->dim1 = n;
          entry->variant_opc_hash->lbound1 = 0;

          for (int i = 0; i < n; i++) {
            entry->variant_type_oids->values[i] = DatumGetObjectId(elems[i]);
            entry->variant_eq_ops->values[i] =
                lookup_operator(entry->variant_type_oids->values[i], "=");
            Oid btree_am_oid = get_am_oid("btree", false);
            entry->variant_opc_btree->values[i] =
                GetDefaultOpClass(entry->variant_type_oids->values[i], btree_am_oid);
            Oid hash_am_oid = get_am_oid("hash", false);
            entry->variant_opc_hash->values[i] =
                GetDefaultOpClass(entry->variant_type_oids->values[i], hash_am_oid);
          }

          matching_entry_found = true;
          break;
        }
      }
      if (scan->rs_rd->rd_tableam->scan_end) {
        scan->rs_rd->rd_tableam->scan_end(scan);
      }
      table_close(rel, AccessShareLock);
      if (!matching_entry_found) {
        sumtypehash_delete(sumtypehash, sum_type_oid);
        break;
      }
    } else {
      if (entry->xmin != xmin) {
        pfree(entry->variant_eq_ops);
        pfree(entry->variant_type_oids);
        sumtypehash_delete(sumtypehash, sum_type_oid);
      } else {
        matching_entry_found = true;
      }
    }
  }
  return matching_entry_found ? entry : NULL;
}

static void get_variant_val(Datum arg, Oid sum_type_oid, Oid *variant, Datum *val,
                            Discriminant *discriminant) {
  HeapTuple sum_type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(sum_type_oid));
  Assert(HeapTupleIsValid(sum_type_tup));
  Form_pg_type sum_typtup = (Form_pg_type)GETSTRUCT(sum_type_tup);
  int16 sum_type_len = sum_typtup->typlen;
  ReleaseSysCache(sum_type_tup);

  VarSizeVariant *varsize =
      sum_type_len == -1 ? (VarSizeVariant *)VARDATA_ANY(PG_DETOAST_DATUM_PACKED(arg)) : NULL;
  FixedSizeVariant *value =
      sum_type_len == -1 ? (FixedSizeVariant *)varsize : (FixedSizeVariant *)DatumGetPointer(arg);

  if (discriminant != NULL) {
    *discriminant = value->discriminant;
  }

  // Find the variant
  *variant = InvalidOid;

  SumTypeCacheEntry *entry = lookup_sumtype_cache(sum_type_oid);

  if (entry) {
    *variant = entry->variant_type_oids->values[value->discriminant];
    HeapTuple type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(*variant));
    Assert(HeapTupleIsValid(type_tup));
    Form_pg_type typtup = (Form_pg_type)GETSTRUCT(type_tup);
    bool variant_byval = typtup->typbyval;

    if (val) {
      *val =
          // if sum type is variable size
          (Datum)(sum_type_len == -1
                      // if variant is variable size
                      ? (variant_byval
                             // if variant is passed by value, get it as is
                             ? *(Datum *)VARDATA_ANY(&varsize->data)
                             // otherwise, pass the pointer
                             : PointerGetDatum(&varsize->data))
                      // if variant is passed by value, get it from behind the pointer
                      : (variant_byval ? *((Datum *)value->data)
                                       // otherwise, pass the pointer
                                       : (Datum)value->data));
    }
    ReleaseSysCache(type_tup);
  }
}

enum cmp_operator {
  OP_EQUAL,
  OP_NOT_EQUAL,
  OP_LESS,
  OP_LESS_EQUAL,
  OP_GREATER,
  OP_GREATER_EQUAL,
  OP_LAST
};
static char *operator_names[OP_LAST] = {
    [OP_EQUAL] = "=",       [OP_NOT_EQUAL] = "<>", [OP_LESS] = "<",
    [OP_LESS_EQUAL] = "<=", [OP_GREATER] = ">",    [OP_GREATER_EQUAL] = ">=",
};

static bool sum_binary_op(Oid type, enum cmp_operator op, Datum arg1, Datum arg2, Oid collation) {
  Oid variant1, variant2;
  Datum val1, val2;
  Discriminant discriminant1;
  Discriminant discriminant2;
  get_variant_val(arg1, type, &variant1, &val1, &discriminant1);
  get_variant_val(arg2, type, &variant2, &val2, &discriminant2);

  // Compare the type first
  if (variant1 != variant2) {
    if (op == OP_EQUAL) {
      return false;
    }
    if (op == OP_LESS) {
      return discriminant1 < discriminant2;
    }
    if (op == OP_LESS_EQUAL) {
      return discriminant1 <= discriminant2;
    }
    if (op == OP_GREATER) {
      return discriminant1 >= discriminant2;
    }
    if (op == OP_GREATER_EQUAL) {
      return discriminant1 >= discriminant2;
    }
  }

  Oid op_oid = InvalidOid;

  if (op == OP_EQUAL) {
    SumTypeCacheEntry *entry = sumtypehash_lookup(sumtypehash, type);
    if (entry != NULL) {
      op_oid = entry->variant_eq_ops->values[discriminant1];
    }
  } else {
    op_oid = lookup_operator(variant1, operator_names[op]);
  }

  if (!OidIsValid(op_oid)) {
    ereport(ERROR, (errmsg("operator %s does not exist", operator_names[op])));
  }

  RegProcedure eq_func_oid = get_opcode(op_oid);
  if (!OidIsValid(eq_func_oid))
    ereport(ERROR,
            (errmsg("operator %s does not have a valid underlying function", operator_names[op])));

  FmgrInfo eq_fmgr;
  fmgr_info(eq_func_oid, &eq_fmgr);

  Datum result = FunctionCall2Coll(&eq_fmgr, collation, val1, val2);
  return DatumGetBool(result);
}

static bool sum_equal(Oid type, Datum arg1, Datum arg2, Oid collation) {
  return sum_binary_op(type, OP_EQUAL, arg1, arg2, collation);
}

static Datum sum_compare(Oid type, Datum arg1, Datum arg2, Oid collation) {
  Oid variant1, variant2;
  Datum val1, val2;
  Discriminant discriminant1, discriminant2;
  get_variant_val(arg1, type, &variant1, &val1, &discriminant1);
  get_variant_val(arg2, type, &variant2, &val2, &discriminant2);

  // Compare the type first
  if (variant1 != variant2) {
    PG_RETURN_INT32(discriminant1 < discriminant2 ? -1 : 1);
  }

  Oid opclass = InvalidOid;
  SumTypeCacheEntry *entry = sumtypehash_lookup(sumtypehash, type);
  if (entry != NULL) {
    opclass = entry->variant_opc_btree->values[discriminant1];
  } else {
    Oid btree_am_oid = get_am_oid("btree", false);
    opclass = GetDefaultOpClass(variant1, btree_am_oid);
  }

  if (!OidIsValid(opclass))
    ereport(ERROR, errmsg("No default btree opclass for type %u", variant1));

  // For btree opclasses, support function number 1 is the cmp function.
  Oid operand = (get_typtype(variant1) == TYPTYPE_COMPOSITE) ? RECORDOID : variant1;
  Oid cmp_proc_oid = get_opfamily_proc(get_opclass_family(opclass), operand, operand, 1);

  if (!OidIsValid(cmp_proc_oid))
    ereport(ERROR, errmsg("No cmp function found in opclass %u for type %u", opclass, variant1));
  return OidFunctionCall2Coll(cmp_proc_oid, collation, val1, val2);
}

Datum sum_eq(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Datum arg2 = PG_GETARG_DATUM(1);

  return sum_equal(type, arg1, arg2, PG_GET_COLLATION());
}

Datum sum_neq(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Datum arg2 = PG_GETARG_DATUM(1);

  return !sum_equal(type, arg1, arg2, PG_GET_COLLATION());
}

Datum sum_lt(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Datum arg2 = PG_GETARG_DATUM(1);
  PG_RETURN_BOOL(sum_binary_op(type, OP_LESS, arg1, arg2, PG_GET_COLLATION()));
}

Datum sum_lte(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Datum arg2 = PG_GETARG_DATUM(1);
  PG_RETURN_BOOL(sum_binary_op(type, OP_LESS_EQUAL, arg1, arg2, PG_GET_COLLATION()));
}

Datum sum_gt(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Datum arg2 = PG_GETARG_DATUM(1);
  PG_RETURN_BOOL(sum_binary_op(type, OP_GREATER, arg1, arg2, PG_GET_COLLATION()));
}

Datum sum_gte(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Datum arg2 = PG_GETARG_DATUM(1);
  PG_RETURN_BOOL(sum_binary_op(type, OP_GREATER_EQUAL, arg1, arg2, PG_GET_COLLATION()));
}

Datum sum_cmp(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Datum arg2 = PG_GETARG_DATUM(1);

  return sum_compare(type, arg1, arg2, PG_GET_COLLATION());
}

Datum sum_hash(PG_FUNCTION_ARGS) {
  Oid type = get_fn_expr_argtype_reliable(fcinfo->flinfo, 0);

  Datum arg1 = PG_GETARG_DATUM(0);
  Oid variant1;
  Datum val1;
  Discriminant discriminant;
  get_variant_val(arg1, type, &variant1, &val1, &discriminant);

  Oid opclass = InvalidOid;
  SumTypeCacheEntry *entry = sumtypehash_lookup(sumtypehash, type);
  if (entry != NULL) {
    opclass = entry->variant_opc_hash->values[discriminant];
  } else {
    Oid hash_am_oid = get_am_oid("hash", false);
    opclass = GetDefaultOpClass(variant1, hash_am_oid);
  }

  if (!OidIsValid(opclass))
    ereport(ERROR, errmsg("No default hash opclass for type %u", variant1));

  // For hash opclasses, support function number 1 is the hash function.
  Oid hash_proc_oid = get_opfamily_proc(get_opclass_family(opclass), variant1, variant1, 1);

  if (!OidIsValid(hash_proc_oid))
    ereport(ERROR, errmsg("No hash function found in opclass %u for type %u", opclass, variant1));

  int h = DatumGetInt32(OidFunctionCall1Coll(hash_proc_oid, PG_GET_COLLATION(), val1));
  PG_RETURN_INT32(h ^ (discriminant + 0x9e3779b9 + (h << 6) + (h >> 2)));
}

Datum sum_type(PG_FUNCTION_ARGS) {

  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("sum type must have a non-NULL name"));
  }

  Name name = PG_GETARG_NAME(0);

  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("sum type must have a non-NULL array of variants"));
  }

  // Get current schema
  List *search_path = fetch_search_path(false);
  Assert(search_path != NIL);
  Oid namespace = linitial_oid(search_path);

  // Obtain `probin` (current extension's binary)
  HeapTuple proc_tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(fcinfo->flinfo->fn_oid));
  Assert(HeapTupleIsValid(proc_tuple));
  bool isnull;
  char *probin = text_to_cstring(
      DatumGetTextPP(SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_probin, &isnull)));
  ReleaseSysCache(proc_tuple);

  ArrayType *arr = PG_GETARG_ARRAYTYPE_P(1);

  int16 variant_size = 0;
  bool varlen = false;
  bool binary_io = true;

  bool can_hash = true;
  bool can_cmp = true;

  // Figure out sum type size, availability of binary I/O, hashability, etc.
  {
    ArrayIterator it = array_create_iterator(arr, 0, NULL);

    Datum elem;
    bool is_null = false;

    while (array_iterate(it, &elem, &is_null)) {
      if (is_null) {
        // Handle null element
        continue;
      }
      HeapTuple type_tuple;
      Form_pg_type typ;

      // Get the type tuple corresponding to the OID
      type_tuple = SearchSysCache1(TYPEOID, elem);
      if (!HeapTupleIsValid(type_tuple)) {
        // Handle error
        ereport(ERROR, errmsg("Type OID %lu is invalid", elem));
      }

      /* Get the Form_pg_type struct from the type tuple */
      typ = (Form_pg_type)GETSTRUCT(type_tuple);

      if (typ->typcategory == TYPCATEGORY_PSEUDOTYPE) {
        ereport(ERROR, errmsg("Pseudo-types can't be variants"),
                errdetail("%s", NameStr(typ->typname)));
      }

      if (typ->typlen == -1) {
        varlen = true;
      } else {
        variant_size = Max(variant_size, typ->typlen);
      }

      // All variants must support binary I/O in order for the sum type to support it
      if (typ->typsend == InvalidOid || typ->typreceive == InvalidOid) {
        binary_io = false;
      }

      {
        Oid hash_am_oid = get_am_oid("hash", false);
        Oid opclass = GetDefaultOpClass(DatumGetObjectId(elem), hash_am_oid);
        // All variants must be hashable to support hashing for the type
        if (get_typtype(DatumGetObjectId(elem)) == TYPTYPE_COMPOSITE || !OidIsValid(opclass)) {
          can_hash = false;
        }
      }

      {
        Oid btree_am_oid = get_am_oid("btree", false);
        Oid opclass = GetDefaultOpClass(DatumGetObjectId(elem), btree_am_oid);
        // All variants must be hashable to support hashing for the type
        if (!OidIsValid(opclass)) {
          can_cmp = false;
        }
      }

      ReleaseSysCache(type_tuple);
    }

    array_free_iterator(it);
  }

  int16 type_size = varlen ? -1 : sizeof(Discriminant) + variant_size;

  // Create shell type first
  ObjectAddress shell = TypeShellMake(NameStr(*name), namespace, GetUserId());

  Oid array_oid = AssignTypeArrayOid();

  // Prepare cstring -> sum type function
  char *f_in = psprintf("%s_in", NameStr(*name));
  ObjectAddress in =
      ProcedureCreate(f_in, namespace, false, false, shell.objectId, GetUserId(), ClanguageId,
                      F_FMGR_C_VALIDATOR, "sum_in", probin,
#if PG_MAJORVERSION_NUM > 13
                      NULL,
#endif
                      PROKIND_FUNCTION, false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                      buildoidvector((Oid[1]){CSTRINGOID}, 1), PointerGetDatum(NULL),
                      PointerGetDatum(NULL), PointerGetDatum(NULL), NIL, PointerGetDatum(NULL),
                      PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

  // Prepare sum type -> cstring function
  char *f_out = psprintf("%s_out", NameStr(*name));
  ObjectAddress out =
      ProcedureCreate(f_out, namespace, false, false, CSTRINGOID, GetUserId(), ClanguageId,
                      F_FMGR_C_VALIDATOR, "sum_out", probin,
#if PG_MAJORVERSION_NUM > 13
                      NULL,
#endif
                      PROKIND_FUNCTION, false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                      buildoidvector((Oid[1]){shell.objectId}, 1), PointerGetDatum(NULL),
                      PointerGetDatum(NULL), PointerGetDatum(NULL), NIL, PointerGetDatum(NULL),
                      PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

  // Prepare send/recv functions (if possible)
  Oid typsend = InvalidOid;
  Oid typrecv = InvalidOid;
  if (binary_io) {
    char *f_recv = psprintf("%s_recv", NameStr(*name));
    ObjectAddress recv =
        ProcedureCreate(f_recv, namespace, false, false, shell.objectId, GetUserId(), ClanguageId,
                        F_FMGR_C_VALIDATOR, "sum_recv", probin,
#if PG_MAJORVERSION_NUM > 13
                        NULL,
#endif
                        PROKIND_FUNCTION, false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                        buildoidvector((Oid[1]){BYTEAOID}, 1), PointerGetDatum(NULL),
                        PointerGetDatum(NULL), PointerGetDatum(NULL), NIL, PointerGetDatum(NULL),
                        PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

    char *f_send = psprintf("%s_send", NameStr(*name));
    ObjectAddress send =
        ProcedureCreate(f_send, namespace, false, false, BYTEAOID, GetUserId(), ClanguageId,
                        F_FMGR_C_VALIDATOR, "sum_send", probin,
#if PG_MAJORVERSION_NUM > 13
                        NULL,
#endif
                        PROKIND_FUNCTION, false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                        buildoidvector((Oid[1]){shell.objectId}, 1), PointerGetDatum(NULL),
                        PointerGetDatum(NULL), PointerGetDatum(NULL), NIL, PointerGetDatum(NULL),
                        PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);
    typrecv = recv.objectId;
    typsend = send.objectId;
  }

  // Prepare the type itself
  char storage = type_size == -1 ? TYPSTORAGE_EXTENDED : TYPSTORAGE_PLAIN;
  ObjectAddress type =
      TypeCreate(InvalidOid, NameStr(*name), namespace, InvalidOid, 0, GetUserId(), type_size,
                 TYPTYPE_BASE, TYPCATEGORY_USER, true, DEFAULT_TYPDELIM, in.objectId, out.objectId,
                 typrecv, typsend, InvalidOid, InvalidOid, InvalidOid,
#if PG_MAJORVERSION_NUM > 13
                 InvalidOid,
#endif
                 InvalidOid, false, array_oid, InvalidOid, NULL, NULL, false, TYPALIGN_INT, storage,
                 -1, 0, false, DEFAULT_COLLATION_OID);

  Assert(shell.objectId == type.objectId);

  char *array_type_name = makeArrayTypeName(NameStr(*name), namespace);

  ObjectAddress array_type =
      TypeCreate(array_oid, array_type_name, namespace, InvalidOid, 0, GetUserId(), -1,
                 TYPTYPE_BASE, TYPCATEGORY_ARRAY, false, DEFAULT_TYPDELIM, F_ARRAY_IN, F_ARRAY_OUT,
                 F_ARRAY_RECV, F_ARRAY_SEND, InvalidOid, InvalidOid, F_ARRAY_TYPANALYZE,
#if PG_MAJORVERSION_NUM > 13
                 F_ARRAY_SUBSCRIPT_HANDLER,
#endif
                 type.objectId, true, InvalidOid, InvalidOid, NULL, NULL, false, TYPALIGN_INT,
                 TYPSTORAGE_EXTENDED, -1, 0, false, DEFAULT_COLLATION_OID);

  // Create hashing function
  if (can_hash) {
    char *hash_sum = psprintf("%s_hash", NameStr(*name));
    ProcedureCreate(hash_sum, namespace, false, false, INT4OID, GetUserId(), ClanguageId,
                    F_FMGR_C_VALIDATOR, "sum_hash", probin,
#if PG_MAJORVERSION_NUM > 13
                    NULL,
#endif
                    PROKIND_FUNCTION,

                    false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                    buildoidvector((Oid[1]){type.objectId}, 1), PointerGetDatum(NULL),
                    PointerGetDatum(NULL), PointerGetDatum(NULL), NIL, PointerGetDatum(NULL),
                    PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);
  }

  // Create comparison operators
  {
    char *eq_sum = psprintf("%s_eq", NameStr(*name));
    ObjectAddress eq_sum_proc =
        ProcedureCreate(eq_sum, namespace, false, false, BOOLOID, GetUserId(), ClanguageId,
                        F_FMGR_C_VALIDATOR, "sum_eq", probin,
#if PG_MAJORVERSION_NUM > 13
                        NULL,
#endif
                        PROKIND_FUNCTION,

                        false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                        buildoidvector((Oid[2]){type.objectId, type.objectId}, 2),
                        PointerGetDatum(NULL), PointerGetDatum(NULL), PointerGetDatum(NULL), NIL,
                        PointerGetDatum(NULL), PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

    ObjectAddress eqop =
        OperatorCreate("=", namespace, type.objectId, type.objectId, eq_sum_proc.objectId,
                       list_make1(makeString("=")), list_make1(makeString("<>")), InvalidOid,
                       InvalidOid, can_cmp, can_hash);

    char *neq_sum = psprintf("%s_neq", NameStr(*name));
    ObjectAddress neq_sum_proc =
        ProcedureCreate(neq_sum, namespace, false, false, BOOLOID, GetUserId(), ClanguageId,
                        F_FMGR_C_VALIDATOR, "sum_neq", probin,
#if PG_MAJORVERSION_NUM > 13
                        NULL,
#endif
                        PROKIND_FUNCTION,

                        false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                        buildoidvector((Oid[2]){type.objectId, type.objectId}, 2),
                        PointerGetDatum(NULL), PointerGetDatum(NULL), PointerGetDatum(NULL), NIL,
                        PointerGetDatum(NULL), PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);
    ObjectAddress neqop =
        OperatorCreate("<>", namespace, type.objectId, type.objectId, neq_sum_proc.objectId,
                       list_make1(makeString("<>")), list_make1(makeString("=")), InvalidOid,
                       InvalidOid, can_cmp, can_hash);

    // Create comparison function
    if (can_cmp) {
      char *cmp_sum = psprintf("%s_cmp", NameStr(*name));
      ProcedureCreate(cmp_sum, namespace, false, false, INT4OID, GetUserId(), ClanguageId,
                      F_FMGR_C_VALIDATOR, "sum_cmp", probin,
#if PG_MAJORVERSION_NUM > 13
                      NULL,
#endif
                      PROKIND_FUNCTION,

                      false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                      buildoidvector((Oid[2]){type.objectId, type.objectId}, 2),
                      PointerGetDatum(NULL), PointerGetDatum(NULL), PointerGetDatum(NULL), NIL,
                      PointerGetDatum(NULL), PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

      char *lt_sum = psprintf("%s_lt", NameStr(*name));
      ObjectAddress lt_proc =
          ProcedureCreate(lt_sum, namespace, false, false, BOOLOID, GetUserId(), ClanguageId,
                          F_FMGR_C_VALIDATOR, "sum_lt", probin,
#if PG_MAJORVERSION_NUM > 13
                          NULL,
#endif
                          PROKIND_FUNCTION,

                          false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                          buildoidvector((Oid[2]){type.objectId, type.objectId}, 2),
                          PointerGetDatum(NULL), PointerGetDatum(NULL), PointerGetDatum(NULL), NIL,
                          PointerGetDatum(NULL), PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

      ObjectAddress ltop =
          OperatorCreate("<", namespace, type.objectId, type.objectId, lt_proc.objectId,
                         list_make1(makeString(">")), list_make1(makeString(">=")), InvalidOid,
                         InvalidOid, can_cmp, can_hash);

      char *lte_sum = psprintf("%s_lte", NameStr(*name));
      ObjectAddress lte_proc =
          ProcedureCreate(lte_sum, namespace, false, false, BOOLOID, GetUserId(), ClanguageId,
                          F_FMGR_C_VALIDATOR, "sum_lte", probin,
#if PG_MAJORVERSION_NUM > 13
                          NULL,
#endif
                          PROKIND_FUNCTION,

                          false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                          buildoidvector((Oid[2]){type.objectId, type.objectId}, 2),
                          PointerGetDatum(NULL), PointerGetDatum(NULL), PointerGetDatum(NULL), NIL,
                          PointerGetDatum(NULL), PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

      ObjectAddress lteop =
          OperatorCreate("<=", namespace, type.objectId, type.objectId, lte_proc.objectId,
                         list_make1(makeString(">=")), list_make1(makeString(">")), InvalidOid,
                         InvalidOid, can_cmp, can_hash);

      char *gt_sum = psprintf("%s_gt", NameStr(*name));
      ObjectAddress gt_proc =
          ProcedureCreate(gt_sum, namespace, false, false, BOOLOID, GetUserId(), ClanguageId,
                          F_FMGR_C_VALIDATOR, "sum_gt", probin,
#if PG_MAJORVERSION_NUM > 13
                          NULL,
#endif
                          PROKIND_FUNCTION,

                          false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                          buildoidvector((Oid[2]){type.objectId, type.objectId}, 2),
                          PointerGetDatum(NULL), PointerGetDatum(NULL), PointerGetDatum(NULL), NIL,
                          PointerGetDatum(NULL), PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

      ObjectAddress gtop =
          OperatorCreate(">", namespace, type.objectId, type.objectId, gt_proc.objectId,
                         list_make1(makeString("<")), list_make1(makeString("<=")), InvalidOid,
                         InvalidOid, can_cmp, can_hash);

      char *gte_sum = psprintf("%s_gte", NameStr(*name));
      ObjectAddress gte_proc =
          ProcedureCreate(gte_sum, namespace, false, false, BOOLOID, GetUserId(), ClanguageId,
                          F_FMGR_C_VALIDATOR, "sum_gte", probin,
#if PG_MAJORVERSION_NUM > 13
                          NULL,
#endif
                          PROKIND_FUNCTION,

                          false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                          buildoidvector((Oid[2]){type.objectId, type.objectId}, 2),
                          PointerGetDatum(NULL), PointerGetDatum(NULL), PointerGetDatum(NULL), NIL,
                          PointerGetDatum(NULL), PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

      ObjectAddress gteop =
          OperatorCreate(">=", namespace, type.objectId, type.objectId, gte_proc.objectId,
                         list_make1(makeString("<=")), list_make1(makeString("<")), InvalidOid,
                         InvalidOid, can_cmp, can_hash);
    }
  }

  // Register the type and ensure constraints are checked
  SPI_connect();
  SPI_execute_with_args("insert into omni_types.sum_types (typ, variants) values ($1, $2)", 2,
                        (Oid[2]){REGTYPEOID, REGTYPEARRAYOID},
                        (Datum[2]){
                            shell.objectId,
                            PointerGetDatum(arr),
                        },
                        (char[2]){' ', ' '}, false, 0);
  SPI_finish();

  // Create casts
  {
    Datum elem;
    bool is_null;
    ArrayIterator it = array_create_iterator(arr, 0, NULL);
    while (array_iterate(it, &elem, &is_null)) {
      if (is_null) {
        // Handle null element
        continue;
      }
      Oid target = DatumGetObjectId(elem);

      HeapTuple type_tuple = SearchSysCache1(TYPEOID, elem);
      Assert(HeapTupleIsValid(type_tuple));

      Form_pg_type typ = (Form_pg_type)GETSTRUCT(type_tuple);
      bool is_domain = typ->typtype == TYPTYPE_DOMAIN;

      char *f_cast_to = psprintf("%s_from_%s", NameStr(typ->typname), NameStr(*name));
      char *f_cast_from = psprintf("%s_from_%s", NameStr(*name), NameStr(typ->typname));

      ReleaseSysCache(type_tuple);

      ObjectAddress cast_to =
          ProcedureCreate(f_cast_to, namespace, false, false, target, GetUserId(), ClanguageId,
                          F_FMGR_C_VALIDATOR, "sum_cast_to", probin,
#if PG_MAJORVERSION_NUM > 13
                          NULL,
#endif
                          PROKIND_FUNCTION,

                          false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                          buildoidvector((Oid[1]){type.objectId}, 1), PointerGetDatum(NULL),
                          PointerGetDatum(NULL), PointerGetDatum(NULL), NIL, PointerGetDatum(NULL),
                          PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

      ObjectAddress cast_from =
          ProcedureCreate(f_cast_from, namespace, false, false, type.objectId, GetUserId(),
                          ClanguageId, F_FMGR_C_VALIDATOR, "sum_cast_from", probin,
#if PG_MAJORVERSION_NUM > 13
                          NULL,
#endif
                          PROKIND_FUNCTION,

                          false, true, true, PROVOLATILE_STABLE, PROPARALLEL_SAFE,
                          buildoidvector((Oid[1]){target}, 1), PointerGetDatum(NULL),
                          PointerGetDatum(NULL), PointerGetDatum(NULL), NIL, PointerGetDatum(NULL),
                          PointerGetDatum(NULL), InvalidOid, 1.0, 0.0);

      if (!is_domain) {
        // It's pointless to create domain casts
        CastCreate(type.objectId, target, cast_to.objectId,
#if PG_MAJORVERSION_NUM >= 16
                   InvalidOid, InvalidOid,
#endif
                   COERCION_CODE_ASSIGNMENT, COERCION_METHOD_FUNCTION, DEPENDENCY_NORMAL);
        CastCreate(target, type.objectId, cast_from.objectId,
#if PG_MAJORVERSION_NUM >= 16
                   InvalidOid, InvalidOid,
#endif
                   COERCION_CODE_ASSIGNMENT, COERCION_METHOD_FUNCTION, DEPENDENCY_NORMAL);
      }
    }

    array_free_iterator(it);
  }

  PG_RETURN_OID(type.objectId);
}

Datum add_variant(PG_FUNCTION_ARGS) {

  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("sum type must have a non-NULL value"));
  }

  Oid sum_type_oid = PG_GETARG_OID(0);

  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("variant type must have a non-NULL value"));
  }

  Oid variant_type_oid = PG_GETARG_OID(1);

  HeapTuple sum_type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(sum_type_oid));
  Assert(HeapTupleIsValid(sum_type_tup));
  Form_pg_type sum_typtup = (Form_pg_type)GETSTRUCT(sum_type_tup);
  int16 sum_type_len = sum_typtup->typlen;
  ReleaseSysCache(sum_type_tup);

  HeapTuple variant_type_tup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(variant_type_oid));
  Assert(HeapTupleIsValid(variant_type_tup));
  Form_pg_type variant_typtup = (Form_pg_type)GETSTRUCT(variant_type_tup);
  int16 variant_type_len = variant_typtup->typlen;
  ReleaseSysCache(variant_type_tup);

  if (sum_type_len != -1 &&
      (variant_type_len < 0 || variant_type_len > (sum_type_len - sizeof(Discriminant)))) {
    // If sum type is fixed size, the variant must not be larger
    ereport(ERROR,
            errmsg("variant type size must not be larger than that of the largest existing variant "
                   "type's"),
            errdetail("largest existing variant size: %lu, variant type size: %d",
                      sum_type_len - sizeof(Discriminant), variant_type_len));
  }

  SPI_connect();
  SPI_execute_with_args(
      "update omni_types.sum_types set variants = array_append(variants, $1) where typ = $2", 2,
      (Oid[2]){REGTYPEOID, REGTYPEOID}, (Datum[2]){variant_type_oid, sum_type_oid},
      (char[2]){' ', ' '}, false, 0);
  SPI_finish();

  PG_RETURN_VOID();
}
