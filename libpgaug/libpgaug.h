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
#define WITH_TEMP_MEMCXT                                                       \
  for (__attribute__((                                                         \
           cleanup(__with_temp_memcxt_cleanup))) struct __with_temp_memcxt     \
           memory_context = {.new = AllocSetContextCreate(                     \
                                 CurrentMemoryContext,                         \
                                 _PGEXT_STRINGIZE(                             \
                                     _PGEXT_PPCAT(__FILE__, __LINE__)),        \
                                 ALLOCSET_DEFAULT_SIZES),                      \
                            .old = MemoryContextSwitchTo(memory_context.new),  \
                            .__phase = _memcxt_EXECUTE};                       \
       memory_context.__phase <= _memcxt_DONE; memory_context.__phase++)       \
    if (memory_context.__phase == _memcxt_SWITCH) {                            \
      MemoryContextSwitchTo(memory_context.old);                               \
    } else if (memory_context.__phase == _memcxt_EXECUTE)

#define MEMCXT_FINALIZE else if (memory_context.__phase == _memcxt_DONE)
#endif // LIBPGAUG_H
