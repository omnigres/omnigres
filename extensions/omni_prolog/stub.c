#include <SWI-Prolog.h>

#include "predicates.h"

foreign_t arg(term_t argument, term_t value) {}
foreign_t query(term_t query, term_t args, term_t out, control_t handle) {}
foreign_t message_hook(term_t term, term_t kind, term_t lines) {}

bool is_stub = true;