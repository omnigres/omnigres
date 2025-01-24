#ifndef LIBPGAUG_H
#define LIBPGAUG_H

#include <postgres.h>
#include <utils/memutils.h>

void *pgaug_alloc(size_t size);
void *pgaug_calloc(uintptr_t num, uintptr_t count);
void pgaug_free(void *ptr);
void *pgaug_realloc(void *ptr, uintptr_t size);

#define _PGEXT_PPCAT_(A, B) A##B
#define _PGEXT_STRINGIZE(A) #A

struct __with_temp_memcxt {
  MemoryContext new;
  MemoryContext old;
  enum { _memcxt_EXECUTE = 0, _memcxt_SWITCH = 1, _memcxt_DONE = 2 } __phase;
};

void __with_temp_memcxt_cleanup(struct __with_temp_memcxt *s);

/**
 * @brief Defines a scoped temporary memory context
 *
 * Exposes `memory_context.new` for the newly created context and
 * `memory_context.old` for the one prior to the start of the context.
 *
 * The code that follows this macro is the code to be executed, and it can be
 * optionally appended with `MEMCXT_FINALIZE` branch to run code when the
 * switched back to the old context but before the deletion of the new context.
 *
 */
// Note: we're not using __func__ because it's not a constant, therefore
// we have to use something else, in this case, it's a file:line.
// That's also better if there's more than one use of this macro in the same
// function.
#define WITH_TEMP_MEMCXT                                                                           \
  for (__attribute__((                                                                             \
           cleanup(__with_temp_memcxt_cleanup))) struct __with_temp_memcxt memory_context =        \
           {.new = AllocSetContextCreate(CurrentMemoryContext,                                     \
                                         _PGEXT_STRINGIZE(_PGEXT_PPCAT(__FILE__, __LINE__)),       \
                                         ALLOCSET_DEFAULT_SIZES),                                  \
           .old = MemoryContextSwitchTo(memory_context.new),                                       \
           .__phase = _memcxt_EXECUTE};                                                            \
       memory_context.__phase <= _memcxt_DONE; memory_context.__phase++)                           \
    if (memory_context.__phase == _memcxt_SWITCH) {                                                \
      MemoryContextSwitchTo(memory_context.old);                                                   \
    } else if (memory_context.__phase == _memcxt_EXECUTE)

#define MEMCXT_FINALIZE else if (memory_context.__phase == _memcxt_DONE)

/**
 * @brief Defines <name>_oid() function that returns the OID of type <name>
 *
 * It also defines <name>_array_oid() function that returns the OID of the array of the type.
 *
 * It is resolved against the schema of the current extension being compiled.
 * The result is cached (as can be implied from the name of the macro)
 *
 */

#define CACHED_OID_2(ext, name)                                                                    \
  static Oid oid_##name = InvalidOid;                                                              \
  static Oid oid_array_##name = InvalidOid;                                                        \
  Oid name##_oid() {                                                                               \
    if (oid_##name == InvalidOid) {                                                                \
      SPI_connect();                                                                               \
      if (SPI_exec("SELECT NULL::" _PGEXT_STRINGIZE(ext) "." _PGEXT_STRINGIZE(name), 0) ==         \
          SPI_OK_SELECT) {                                                                         \
        TupleDesc tupdesc = SPI_tuptable->tupdesc;                                                 \
        oid_##name = TupleDescAttr(tupdesc, 0)->atttypid;                                          \
      }                                                                                            \
      SPI_finish();                                                                                \
    }                                                                                              \
    return oid_##name;                                                                             \
  }                                                                                                \
  Oid name##_array_oid() {                                                                         \
    if (oid_array_##name == InvalidOid) {                                                          \
      SPI_connect();                                                                               \
      if (SPI_exec("SELECT array[]::" _PGEXT_STRINGIZE(ext) "." _PGEXT_STRINGIZE(name) "[]", 0) == \
          SPI_OK_SELECT) {                                                                         \
        TupleDesc tupdesc = SPI_tuptable->tupdesc;                                                 \
        oid_array_##name = TupleDescAttr(tupdesc, 0)->atttypid;                                    \
      }                                                                                            \
      SPI_finish();                                                                                \
    }                                                                                              \
    return oid_array_##name;                                                                       \
  }

#define CACHED_OID_1(name) CACHED_OID_2(EXT_SCHEMA, name)

#define CACHED_OID_GET_MACRO(_1, _2, NAME, ...) NAME
#define CACHED_OID(...) CACHED_OID_GET_MACRO(__VA_ARGS__, CACHED_OID_2, CACHED_OID_1)(__VA_ARGS__)

#define CACHED_ENUM_OID(type, label)                                                               \
  Oid type##_##label##_oid() {                                                                     \
    static HeapTuple tup = NULL;                                                                   \
    static Oid oid = InvalidOid;                                                                   \
                                                                                                   \
    if (tup == NULL) {                                                                             \
      tup = SearchSysCache2(ENUMTYPOIDNAME, ObjectIdGetDatum(type##_oid()),                        \
                            CStringGetDatum(_PGEXT_STRINGIZE(label)));                             \
      if (!HeapTupleIsValid(tup)) {                                                                \
        ereport(ERROR, errmsg("invalid enum label '" _PGEXT_STRINGIZE(                             \
                           label) "' for type '" _PGEXT_STRINGIZE(type) "'"));                     \
      }                                                                                            \
      oid = ((Form_pg_enum)GETSTRUCT(tup))->oid;                                                   \
      ReleaseSysCache(tup);                                                                        \
    }                                                                                              \
    return oid;                                                                                    \
  }

#define GetAttributeByIndex(t, index, isnull) GetAttributeByNum(t, index + 1, isnull)

bool IsPostmasterBeingShutdown();

#endif // LIBPGAUG_H
