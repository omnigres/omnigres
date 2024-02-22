#include <stdio.h>

#include <SWI-Prolog.h>

#define PL_require(x)                                                                              \
  if (!x)                                                                                          \
  return FALSE

term_t Logtalk(term_t receiver, char *predicate_name, int arity, term_t arg) {
  // create term for the message
  functor_t functor = PL_new_functor(PL_new_atom(predicate_name), arity);
  term_t pred = PL_new_term_ref();
  PL_require(PL_cons_functor_v(pred, functor, arg));

  // term for ::(receiver, message)
  functor_t send_functor = PL_new_functor(PL_new_atom("::"), 2);
  term_t goal = PL_new_term_ref();
  PL_require(PL_cons_functor(goal, send_functor, receiver, pred));

  return goal;
}

term_t Logtalk_named(char *object_name, char *predicate_name, int arity, term_t arg) {
  // create term for the receiver of the message
  term_t receiver = PL_new_term_ref();
  PL_put_atom_chars(receiver, object_name);

  return Logtalk(receiver, predicate_name, arity, arg);
}

int main(int argc, char **argv) {
  // SWI Prolog
  char *program = argv[0];
  char *plav[2] = {program, NULL};
  if (!PL_initialise(1, plav)) {
    PL_halt(1);
  }

  term_t arg; // empty
  term_t main = Logtalk_named("main", "main", 0, arg);
  if (PL_call(main, NULL) == FALSE) {
    printf("Error calling main entrypoint\n");
  }

  PL_cleanup(PL_CLEANUP_NO_CANCEL);
}