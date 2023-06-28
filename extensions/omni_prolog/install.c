#include <SWI-Prolog.h>

#include "predicates.h"

install_t install() {
  PL_register_foreign("arg", 2, arg, 0);
  PL_register_foreign("query", 3, query, PL_FA_NONDETERMINISTIC);
  if (!is_stub) {
    PL_register_foreign("message_hook", 3, message_hook, 0);
  }
}
