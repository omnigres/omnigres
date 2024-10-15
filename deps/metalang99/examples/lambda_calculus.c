/*
 * An untyped lambda calculus [1] interpreter using De Bruijn indices [2] and normal order
 * evaluation strategy [3].
 *
 * [1]: https://en.wikipedia.org/wiki/Lambda_calculus
 * [2]: https://en.wikipedia.org/wiki/De_Bruijn_index
 * [3]: https://en.wikipedia.org/wiki/Evaluation_strategy#Normal_order
 */

#include <metalang99.h>

// Syntactic terms {

#define var(i)     ML99_call(var, i)
#define appl(M, N) ML99_call(appl, M, N)
#define lam(M)     ML99_call(lam, M)

#define var_IMPL(i)     v(VAR(i))
#define appl_IMPL(M, N) v(APPL(M, N))
#define lam_IMPL(M)     v(LAM(M))

#define VAR(i)     ML99_CHOICE(var, i)
#define APPL(M, N) ML99_CHOICE(appl, M, N)
#define LAM(M)     ML99_CHOICE(lam, M)
// } (Syntactic terms)

// Variable substitution: `M[1=x]` {

#define subst(M, x) ML99_call(subst, M, x)

#define subst_IMPL(M, x)           substAux_IMPL(M, x, 1)
#define substAux_IMPL(M, x, depth) ML99_callUneval(ML99_matchWithArgs, M, substAux_, x, depth)

#define substAux_var_IMPL(i, x, depth)                                                             \
    ML99_IF(                                                                                       \
        ML99_NAT_EQ(i, depth),                                                                     \
        v(x),                                                                                      \
        ML99_call(ML99_if, ML99_callUneval(ML99_greater, i, depth), v(VAR(ML99_DEC(i)), VAR(i))))
#define substAux_appl_IMPL(M, N, x, depth)                                                         \
    appl(substAux_IMPL(M, x, depth), substAux_IMPL(N, x, depth))
#define substAux_lam_IMPL(M, x, depth)                                                             \
    lam(ML99_call(substAux, v(M), incFreeVars_IMPL(x), v(ML99_INC(depth))))
// } (Variable substitution)

// Increment free variables in `M` {

#define incFreeVars(M) ML99_call(incFreeVars, M)

#define incFreeVars_IMPL(M)           incFreeVarsAux_IMPL(M, 1)
#define incFreeVarsAux_IMPL(M, depth) ML99_callUneval(ML99_matchWithArgs, M, incFreeVarsAux_, depth)

#define incFreeVarsAux_var_IMPL(i, depth)                                                          \
    ML99_call(ML99_if, ML99_callUneval(ML99_greaterEq, i, depth), v(VAR(ML99_INC(i)), VAR(i)))
#define incFreeVarsAux_appl_IMPL(M, N, depth)                                                      \
    appl(incFreeVarsAux_IMPL(M, depth), incFreeVarsAux_IMPL(N, depth))
#define incFreeVarsAux_lam_IMPL(M, depth) lam(incFreeVarsAux_IMPL(M, ML99_INC(depth)))
// } (Increment free variables)

// Evaluation {

#define eval(M) ML99_call(eval, M)

#define eval_IMPL(M)         ML99_callUneval(ML99_match, M, eval_)
#define eval_var_IMPL(i)     v(VAR(i))
#define eval_appl_IMPL(M, N) ML99_callUneval(ML99_matchWithArgs, M, eval_appl_, N)
#define eval_lam_IMPL(M)     lam(eval_IMPL(M))

#define eval_appl_var_IMPL(i, N) appl(v(VAR(i)), eval_IMPL(N))
#define eval_appl_appl_IMPL(M, N, N1)                                                              \
    ML99_call(ML99_matchWithArgs, eval(appl_IMPL(M, N)), v(eval_appl_appl_, N1))
#define eval_appl_lam_IMPL(M, N) eval(subst_IMPL(M, N))

#define eval_appl_appl_var_IMPL            eval_appl_var_IMPL
#define eval_appl_appl_appl_IMPL(M, N, N1) appl(appl_IMPL(M, N), eval_IMPL(N1))
#define eval_appl_appl_lam_IMPL            eval_appl_lam_IMPL
// } (Evaluation)

// Syntactical equality {

#define termEq(lhs, rhs)            ML99_matchWithArgs(lhs, v(termEq_), rhs)
#define termEq_var_IMPL(i, rhs)     termEqPropagate(var, rhs, i)
#define termEq_appl_IMPL(M, N, rhs) termEqPropagate(appl, rhs, M, N)
#define termEq_lam_IMPL(M, rhs)     termEqPropagate(lam, rhs, M)

#define termEqPropagate(term_kind, rhs, ...)                                                       \
    ML99_IF(                                                                                       \
        ML99_IDENT_EQ(TERM_, ML99_CHOICE_TAG(rhs), term_kind),                                     \
        ML99_matchWithArgs(v(rhs), v(termEq_##term_kind##_), v(__VA_ARGS__)),                      \
        ML99_false())

#define termEq_var_var_IMPL(j, i)           v(ML99_NAT_EQ(i, j))
#define termEq_appl_appl_IMPL(M, N, M1, N1) ML99_and(termEq(v(M), v(M1)), termEq(v(N), v(N1)))
#define termEq_lam_lam_IMPL(M, M1)          termEq(v(M), v(M1))

#define TERM_var_var   ()
#define TERM_appl_appl ()
#define TERM_lam_lam   ()
// } (Syntactical equality)

#define ASSERT_REDUCES_TO(lhs, rhs)                                                                \
    /* Use two interpreter passes: one for `eval(lhs)`, one for `termEq`. Thereby we achieve more  \
     * Metalang99 reduction steps available. */                                                    \
    ML99_ASSERT_UNEVAL(ML99_EVAL(termEq(v(ML99_EVAL(eval(v(lhs)))), v(ML99_EVAL(eval(v(rhs)))))))

// The identity combinator {

#define I LAM(VAR(1))

ASSERT_REDUCES_TO(APPL(I, VAR(5)), VAR(5));
// } (The identity combinator)

// The K, S combinators {

#define K LAM(LAM(VAR(2)))
#define S LAM(LAM(LAM(APPL(APPL(VAR(3), VAR(1)), APPL(VAR(2), VAR(1))))))

ASSERT_REDUCES_TO(APPL(APPL(S, K), K), I);
ASSERT_REDUCES_TO(APPL(APPL(APPL(S, K), S), K), K);

ASSERT_REDUCES_TO(APPL(APPL(APPL(S, K), VAR(5)), VAR(6)), VAR(6));
ASSERT_REDUCES_TO(APPL(APPL(K, VAR(5)), VAR(6)), VAR(5));
// } (The K, S combinators)

// Church booleans {

#define T LAM(LAM(VAR(2)))
#define F LAM(LAM(VAR(1)))

#define NOT LAM(APPL(APPL(VAR(1), F), T))
#define AND LAM(LAM(APPL(APPL(VAR(2), VAR(1)), VAR(2))))
#define OR  LAM(LAM(APPL(APPL(VAR(2), VAR(2)), VAR(1))))
#define XOR LAM(LAM(APPL(APPL(VAR(2), APPL(NOT, VAR(1))), VAR(1))))

#define IF LAM(LAM(LAM(APPL(APPL(VAR(3), VAR(2)), VAR(1)))))

ASSERT_REDUCES_TO(APPL(NOT, T), F);
ASSERT_REDUCES_TO(APPL(NOT, F), T);
ASSERT_REDUCES_TO(APPL(NOT, APPL(NOT, T)), T);
ASSERT_REDUCES_TO(APPL(NOT, APPL(NOT, F)), F);

ASSERT_REDUCES_TO(APPL(APPL(AND, T), T), T);
ASSERT_REDUCES_TO(APPL(APPL(AND, T), F), F);
ASSERT_REDUCES_TO(APPL(APPL(AND, F), T), F);
ASSERT_REDUCES_TO(APPL(APPL(AND, F), F), F);

ASSERT_REDUCES_TO(APPL(APPL(OR, T), T), T);
ASSERT_REDUCES_TO(APPL(APPL(OR, T), F), T);
ASSERT_REDUCES_TO(APPL(APPL(OR, F), T), T);
ASSERT_REDUCES_TO(APPL(APPL(OR, F), F), F);

ASSERT_REDUCES_TO(APPL(APPL(XOR, T), T), F);
ASSERT_REDUCES_TO(APPL(APPL(XOR, T), F), T);
ASSERT_REDUCES_TO(APPL(APPL(XOR, F), T), T);
ASSERT_REDUCES_TO(APPL(APPL(XOR, F), F), F);

ASSERT_REDUCES_TO(APPL(APPL(APPL(IF, T), VAR(5)), VAR(6)), VAR(5));
ASSERT_REDUCES_TO(APPL(APPL(APPL(IF, F), VAR(5)), VAR(6)), VAR(6));
// } (Church booleans)

// Church numerals {

#define ZERO LAM(LAM(VAR(1)))
#define SUCC LAM(LAM(LAM(APPL(VAR(2), APPL(APPL(VAR(3), VAR(2)), VAR(1))))))

#define ONE   APPL(SUCC, ZERO)
#define TWO   APPL(SUCC, ONE)
#define THREE APPL(SUCC, TWO)
#define FOUR  APPL(SUCC, THREE)

#define ADD LAM(LAM(LAM(LAM(APPL(APPL(VAR(4), VAR(2)), APPL(APPL(VAR(3), VAR(2)), VAR(1)))))))
#define MUL LAM(LAM(LAM(LAM(APPL(APPL(VAR(4), APPL(VAR(3), VAR(2))), VAR(1))))))

ASSERT_REDUCES_TO(APPL(APPL(ADD, ZERO), ZERO), ZERO);
ASSERT_REDUCES_TO(APPL(APPL(ADD, ZERO), ONE), ONE);
ASSERT_REDUCES_TO(APPL(APPL(ADD, ONE), ZERO), ONE);
ASSERT_REDUCES_TO(APPL(APPL(ADD, ONE), TWO), THREE);

ASSERT_REDUCES_TO(APPL(APPL(MUL, ZERO), ZERO), ZERO);
ASSERT_REDUCES_TO(APPL(APPL(MUL, ZERO), ONE), ZERO);
ASSERT_REDUCES_TO(APPL(APPL(MUL, ONE), ZERO), ZERO);
ASSERT_REDUCES_TO(APPL(APPL(MUL, TWO), TWO), FOUR);
// } (Church numerals)

// Church pairs {

#define PAIR LAM(LAM(LAM(APPL(APPL(VAR(1), VAR(3)), VAR(2)))))
#define FST  LAM(APPL(VAR(1), T))
#define SND  LAM(APPL(VAR(1), F))

ASSERT_REDUCES_TO(APPL(FST, APPL(APPL(PAIR, VAR(5)), VAR(6))), VAR(5));
ASSERT_REDUCES_TO(APPL(SND, APPL(APPL(PAIR, VAR(5)), VAR(6))), VAR(6));
// } (Church pairs)

// Church lists {

#define NIL    F
#define CONS   PAIR
#define IS_NIL LAM(APPL(APPL(VAR(1), LAM(LAM(LAM(F)))), T))

#define LIST_1_2_3 APPL(APPL(CONS, VAR(1)), APPL(APPL(CONS, VAR(2)), APPL(APPL(CONS, VAR(3)), NIL)))

ASSERT_REDUCES_TO(APPL(IS_NIL, NIL), T);
ASSERT_REDUCES_TO(APPL(IS_NIL, LIST_1_2_3), F);
// } (Church lists)

// Recursion via self-application (or the Y combinator) is perfectly expressible, though when
// executed, it exhausts the Metalang99 recursion engine limit.

int main(void) {}
