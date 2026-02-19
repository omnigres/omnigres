#include <stdbool.h>

#include <SWI-Prolog.h>

extern bool is_stub;

foreign_t arg(term_t argument, term_t value);
foreign_t query(term_t query, term_t args, term_t out, control_t handle);
foreign_t message_hook(term_t term, term_t kind, term_t lines);

install_t install();