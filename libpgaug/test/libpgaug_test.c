
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <libpgaug.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(switch_to_new);

Datum switch_to_new(PG_FUNCTION_ARGS) {
  bool switched = false;
  MemoryContext old = CurrentMemoryContext;
  WITH_TEMP_MEMCXT { switched = CurrentMemoryContext != old; }
  PG_RETURN_BOOL(switched);
}

PG_FUNCTION_INFO_V1(switch_back_to_old);

Datum switch_back_to_old(PG_FUNCTION_ARGS) {
  bool switched = false;
  MemoryContext old = CurrentMemoryContext;
  WITH_TEMP_MEMCXT {}
  PG_RETURN_BOOL(CurrentMemoryContext == old);
}

PG_FUNCTION_INFO_V1(switch_back_to_old_with_return);

void switch_back_to_old_with_return_() {
  WITH_TEMP_MEMCXT { return; }
}
Datum switch_back_to_old_with_return(PG_FUNCTION_ARGS) {
  bool switched = false;
  MemoryContext old = CurrentMemoryContext;
  switch_back_to_old_with_return_();

  PG_RETURN_BOOL(CurrentMemoryContext == old);
}

PG_FUNCTION_INFO_V1(switch_when_finalizing);

Datum switch_when_finalizing(PG_FUNCTION_ARGS) {
  bool switched = false;
  MemoryContext old = CurrentMemoryContext;
  WITH_TEMP_MEMCXT {}
  MEMCXT_FINALIZE { switched = CurrentMemoryContext == old; }

  PG_RETURN_BOOL(switched);
}
