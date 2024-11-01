// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#ifndef omni_var_h
#define omni_var_h

typedef struct VariableValue {
  bool isnull;
  Oid oid;
  SubTransactionId subxactid; /* Only used for transactional variables */
  Datum value;
  struct VariableValue *previous;
} VariableValue;

typedef struct {
  NameData name;
  // Pointer to the last value
  VariableValue *variable_value;
  // The initial allocation, `variable_value` points to it at the beginning
  // to avoid an additional allocation call
  VariableValue initial_allocation;
} Variable;

extern int num_scope_vars;

#if PG_MAJORVERSION_NUM < 14
#define VAR_HASH_ELEM HASH_ELEM
#else
#define VAR_HASH_ELEM HASH_ELEM | HASH_STRINGS
#endif

#endif // omni_var_h
