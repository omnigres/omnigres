#ifndef libpgaug_H
#define libpgaug_H

#include <postgres.h>
#include <utils/memutils.h>

#define _PGEXT_PPCAT_(A, B) A##B
#define _PGEXT_STRINGIZE(A) #A

/**
 * @brief Starts a temporary memory context
 *
 * Exposes `memory_context` for the current context and `old_memory_context`
 * for the one prior to the start of the context.
 *
 * Must be matched with optional `#WITH_MEMCXT_FINALLY` and mandatory
 * `#WITH_MEMCXT`.
 *
 */
// Note: we're not using __file__ because it's not a constant, therefore
// we have to use something else, in this case, it's a file:line
#define WITH_MEMCXT()                                                          \
  MemoryContext memory_context = AllocSetContextCreate(                        \
      CurrentMemoryContext,                                                    \
      _PGEXT_STRINGIZE(_PGEXT_PPCAT(__FILE__, __LINE__)),                      \
      ALLOCSET_DEFAULT_SIZES);                                                 \
  MemoryContext old_memory_context = MemoryContextSwitchTo(memory_context)

/**
 * @brief Switches to the old memory context before `#END_WITH_MEMCXT`
 *
 * Can be used to allocate in the old context before the new one is deleted.
 *
 */
#define WITH_MEMCXT_FINALLY() MemoryContextSwitchTo(old_memory_context)

/**
 * @brief Ends temporary memory context
 *
 * Switches to the old memory context (if not already) and deletes the temporary
 * memory context.
 *
 */
#define END_WITH_MEMCXT()                                                      \
  if (CurrentMemoryContext == memory_context) {                                \
    MemoryContextSwitchTo(old_memory_context);                                 \
  }                                                                            \
  MemoryContextDelete(memory_context)

#endif // libpgaug_H
