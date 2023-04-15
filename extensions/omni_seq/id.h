#ifndef PREFIX_SIZE
#error "PREFIX_SIZE must be defined"
#endif

#ifndef VAL_SIZE
#error "VAL_SIZE must be defined"
#endif

#define concat_(a, b) a##b
#define concat(a, b) concat_(a, b)

#define PREFIX_TYPE concat(int, PREFIX_SIZE)
#define VAL_TYPE concat(int, VAL_SIZE)

#include <inttypes.h>
#include <stdalign.h>

#ifndef pri_int16_t
#define pri_int16_t PRId16
#endif

#ifndef pri_int32_t
#define pri_int32_t PRId32
#endif

#ifndef pri_int64_t
#define pri_int64_t PRId64
#endif

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <utils/builtins.h>

#define make_name__(name, prefix_len, val_len, suffix) name##_##prefix_len##_##val_len##suffix

#if PREFIX_SIZE == VAL_SIZE
#define make_name_(name, prefix_len, val_len, suffix) name##_##prefix_len##suffix
#else
#define make_name_(name, prefix_len, val_len, suffix) name##_##prefix_len##_##val_len##suffix
#endif
#define make_name(name, prefix_len, val_len) make_name_(name, prefix_len, val_len, )
#define make_name_ref(name, prefix_len, val_len) make_name__(name, prefix_len, val_len, )
#define make_fun_name(name, prefix_len, val_len, suffix)                                           \
  make_name_(name, prefix_len, val_len, suffix)

#ifndef _int16_oid
#define _int16_oid INT2OID
#endif
#ifndef _int32_oid
#define _int32_oid INT4OID
#endif
#ifndef _int64_oid
#define _int64_oid INT8OID
#endif

#ifndef PG_GETARG_int16_
#define PG_GETARG_int16_ PG_GETARG_INT16
#endif

#ifndef PG_GETARG_int32_
#define PG_GETARG_int32_ PG_GETARG_INT32
#endif

#ifndef PG_GETARG_int64_
#define PG_GETARG_int64_ PG_GETARG_INT64
#endif

#define _stringify(x) #x
#define stringify(x) _stringify(x)

typedef struct {
// Bigger field should go first to self-align at the start,
// otherwise it will require padding to align correctly.
#if PREFIX_SIZE >= VAL_SIZE
  PREFIX_TYPE prefix;
  VAL_TYPE val;
#else
  VAL_TYPE val;
  PREFIX_TYPE prefix;
#endif
  // we're packing this struct to ensure no trailing padding
} pg_attribute_packed() make_name(st, PREFIX_TYPE, VAL_TYPE);

StaticAssertDecl(sizeof(make_name(st, PREFIX_TYPE, VAL_TYPE)) ==
                     sizeof(PREFIX_TYPE) + sizeof(VAL_TYPE),
                 "type must not be larger than its components");

StaticAssertDecl(offsetof(make_name(st, PREFIX_TYPE, VAL_TYPE), prefix) % alignof(PREFIX_TYPE) == 0,
                 "prefix must self-align");

StaticAssertDecl(offsetof(make_name(st, PREFIX_TYPE, VAL_TYPE), val) % alignof(VAL_TYPE) == 0,
                 "val must self-align");

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _in));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _in)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  char *input = PG_GETARG_CSTRING(0);
  char *split = strchr(input, ':');

  if (split == NULL) {
    ereport(ERROR, errmsg("invalid input format"));
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val = palloc(sizeof(*val));
  val->prefix = (PREFIX_TYPE)strtoll(input, NULL, 10);
  if (errno == ERANGE) {
    ereport(ERROR, errmsg("range error"));
  }
  val->val = (PREFIX_TYPE)strtoll(split + 1, NULL, 10);
  if (errno == ERANGE) {
    ereport(ERROR, errmsg("range error"));
  }

  PG_RETURN_POINTER(val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _out));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _out)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);

  char *out = psprintf("%" make_name_ref(pri, PREFIX_TYPE, t) ":%" make_name_ref(pri, VAL_TYPE, t),
                       (concat(PREFIX_TYPE, _t))val->prefix, (concat(VAL_TYPE, _t))val->val);

  PG_RETURN_CSTRING(out);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _recv));

#ifndef pg_ntoh_int16
#define pg_ntoh_int16 pg_ntoh16
#endif

#ifndef pg_ntoh_int32
#define pg_ntoh_int32 pg_ntoh32
#endif

#ifndef pg_ntoh_int64
#define pg_ntoh_int64 pg_ntoh64
#endif

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _recv)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }

  bytea *input = PG_GETARG_BYTEA_PP(0);
  if (VARSIZE_ANY_EXHDR(input) != sizeof(PREFIX_TYPE) + sizeof(VAL_TYPE)) {
    ereport(ERROR, errmsg("input length is incorrect"),
            errdetail("expected %ld bytes, got %ld bytes", sizeof(PREFIX_TYPE) + sizeof(VAL_TYPE),
                      VARSIZE_ANY_EXHDR(input)));
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val = palloc(sizeof(make_name(st, PREFIX_TYPE, VAL_TYPE)));
  val->prefix = concat(pg_ntoh_, PREFIX_TYPE)((PREFIX_TYPE) * (PREFIX_TYPE *)VARDATA_ANY(input));
  val->val = concat(pg_ntoh_,
                    VAL_TYPE)((VAL_TYPE) * (VAL_TYPE *)(VARDATA_ANY(input) + sizeof(PREFIX_TYPE)));

  PG_RETURN_POINTER(val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _send));

#ifndef pg_hton_int16
#define pg_hton_int16 pg_hton16
#endif

#ifndef pg_hton_int32
#define pg_hton_int32 pg_hton32
#endif

#ifndef pg_hton_int64
#define pg_hton_int64 pg_hton64
#endif

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _send)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);

  size_t sz = VARHDRSZ + sizeof(val->prefix) + sizeof(val->val);
  struct varlena *bytes = palloc(sz);
  SET_VARSIZE(bytes, sz);

  PREFIX_TYPE prefix = concat(pg_hton_, PREFIX_TYPE)(val->prefix);
  VAL_TYPE value = concat(pg_hton_, VAL_TYPE)(val->val);

  memcpy(VARDATA_ANY(bytes), &prefix, sizeof(prefix));
  memcpy(VARDATA_ANY(bytes) + sizeof(prefix), &value, sizeof(value));

  PG_RETURN_BYTEA_P(bytes);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _eq));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _eq)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val1 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val2 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(1);

  PG_RETURN_BOOL(val1->prefix == val2->prefix && val1->val == val2->val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _neq));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _neq)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val1 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val2 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(1);

  PG_RETURN_BOOL(!(val1->prefix == val2->prefix && val1->val == val2->val));
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _leq));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _leq)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val1 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val2 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(1);

  PG_RETURN_BOOL(val1->prefix <= val2->prefix && val1->val <= val2->val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _lt));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _lt)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val1 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val2 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(1);

  PG_RETURN_BOOL(val1->prefix < val2->prefix && val1->val < val2->val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _geq));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _geq)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val1 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val2 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(1);

  PG_RETURN_BOOL(val1->prefix >= val2->prefix && val1->val >= val2->val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _gt));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _gt)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val1 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val2 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(1);

  PG_RETURN_BOOL(val1->prefix > val2->prefix && val1->val > val2->val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _cmp));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _cmp)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val1 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(0);
  make_name(st, PREFIX_TYPE, VAL_TYPE) *val2 =
      (make_name(st, PREFIX_TYPE, VAL_TYPE) *)PG_GETARG_POINTER(1);

  if (val1->prefix > val2->prefix && val1->val > val2->val) {
    PG_RETURN_INT32(1);
  } else if (val1->prefix < val2->prefix) {
    PG_RETURN_INT32(-1);
  } else {
    if (val1->val > val2->val) {
      PG_RETURN_INT32(1);
    } else if (val1->val < val2->val) {
      PG_RETURN_INT32(-1);
    } else {
      PG_RETURN_INT32(0);
    }
  }
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _nextval));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _nextval)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  Oid sequence_oid = PG_GETARG_OID(1);

  int64 nextval = DirectFunctionCall1(nextval_oid, sequence_oid);

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val = palloc(sizeof(*val));
  val->prefix = make_name_ref(PG_GETARG, PREFIX_TYPE, )(0);
  val->val = (VAL_TYPE)nextval;
  PG_RETURN_POINTER(val);
}

PG_FUNCTION_INFO_V1(make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _make));

Datum make_fun_name(id, PREFIX_TYPE, VAL_TYPE, _make)(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0) || PG_ARGISNULL(1)) {
    PG_RETURN_NULL();
  }

  make_name(st, PREFIX_TYPE, VAL_TYPE) *val = palloc(sizeof(*val));
  val->prefix = make_name_ref(PG_GETARG, PREFIX_TYPE, )(0);
  val->val = (VAL_TYPE)PG_GETARG_INT64(1);
  PG_RETURN_POINTER(val);
}

#undef stringify
#undef _stringify
#undef make_name_
#undef make_name__
#undef make_name
#undef make_fun_name
#undef concat
#undef concat_
#undef VAL_TYPE
#undef PREFIX_TYPE
#undef VAL_SIZE
