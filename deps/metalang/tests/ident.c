#include <metalang99/assert.h>
#include <metalang99/bool.h>
#include <metalang99/ident.h>

int main(void) {

#define EMPTY

#define FOO_x ()
#define FOO_y ()

    // ML99_detectIdent
    {
        ML99_ASSERT(ML99_detectIdent(v(FOO_), v(x)));
        ML99_ASSERT(ML99_detectIdent(v(FOO_), v(y)));
        ML99_ASSERT(ML99_not(ML99_detectIdent(v(FOO_), v(z))));

        ML99_ASSERT(ML99_not(ML99_detectIdent(v(BAR_), v(x))));
        ML99_ASSERT(ML99_not(ML99_detectIdent(v(BAR_), v(abc))));
        ML99_ASSERT(ML99_not(ML99_detectIdent(v(BAR_), v(defghi))));
    }

    // ML99_DETECT_IDENT
    {
        ML99_ASSERT_UNEVAL(ML99_DETECT_IDENT(FOO_, x));
        ML99_ASSERT_UNEVAL(!ML99_DETECT_IDENT(BAR_, x));
    }

#undef FOO_x
#undef FOO_y

#define FOO_x_x ()
#define FOO_y_y ()

    // ML99_identEq
    {
        ML99_ASSERT(ML99_identEq(v(FOO_), v(x), v(x)));
        ML99_ASSERT(ML99_identEq(v(FOO_), v(y), v(y)));

        ML99_ASSERT(ML99_not(ML99_identEq(v(FOO_), v(x), v(y))));
        ML99_ASSERT(ML99_not(ML99_identEq(v(FOO_), v(abc), v(d))));
        ML99_ASSERT(ML99_not(ML99_identEq(v(FOO_), v(x), v(EMPTY))));
        ML99_ASSERT(ML99_not(ML99_identEq(v(FOO_), v(EMPTY), v(y))));
        ML99_ASSERT(ML99_not(ML99_identEq(v(FOO_), v(EMPTY), v(EMPTY))));
    }

    // ML99_IDENT_EQ
    {
        ML99_ASSERT_UNEVAL(ML99_IDENT_EQ(FOO_, x, x));
        ML99_ASSERT_UNEVAL(!ML99_IDENT_EQ(FOO_, x, y));
    }

#undef FOO_x_x
#undef FOO_y_y

    // ML99_charEq
    {
        ML99_ASSERT(ML99_charEq(v(a), v(a)));
        ML99_ASSERT(ML99_charEq(v(x), v(x)));
        ML99_ASSERT(ML99_charEq(v(e), v(e)));

        ML99_ASSERT(ML99_charEq(v(T), v(T)));
        ML99_ASSERT(ML99_charEq(v(J), v(J)));
        ML99_ASSERT(ML99_charEq(v(D), v(D)));

        ML99_ASSERT(ML99_charEq(v(0), v(0)));
        ML99_ASSERT(ML99_charEq(v(5), v(5)));
        ML99_ASSERT(ML99_charEq(v(9), v(9)));

        ML99_ASSERT(ML99_not(ML99_charEq(v(a), v(idf2))));
        ML99_ASSERT(ML99_not(ML99_charEq(v(T), v(abracadabra))));
        ML99_ASSERT(ML99_not(ML99_charEq(v(0), v(123))));
        ML99_ASSERT(ML99_not(ML99_charEq(v(abracadabra), v(abracadabra))));
    }

    // ML99_CHAR_EQ
    {
        ML99_ASSERT_UNEVAL(ML99_CHAR_EQ(x, x));
        ML99_ASSERT_UNEVAL(!ML99_CHAR_EQ(x, 0));
    }

    // ML99_C_KEYWORD_DETECTOR
    {

#define TEST(keyword)                                                                              \
    ML99_ASSERT_UNEVAL(                                                                            \
        ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, keyword, keyword) &&                                \
        !ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, keyword, blah) &&                                  \
        !ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, keyword, EMPTY) &&                                 \
        !ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, EMPTY, keyword))

        TEST(auto);
        TEST(break);
        TEST(case);
        TEST(char);
        TEST(const);
        TEST(continue);
        TEST(default);
        TEST(do);
        TEST(double);
        TEST(else);
        TEST(enum);
        TEST(extern);
        TEST(float);
        TEST(for);
        TEST(goto);
        TEST(if);
        TEST(inline);
        TEST(int);
        TEST(long);
        TEST(register);
        TEST(restrict);
        // clang-format off
        TEST(return);
        // clang-format on
        TEST(short);
        TEST(signed);
        TEST(sizeof);
        TEST(static);
        TEST(struct);
        TEST(switch);
        TEST(typedef);
        TEST(union);
        TEST(unsigned);
        TEST(void);
        TEST(volatile);
        TEST(while);
        TEST(_Alignas);
        TEST(_Alignof);
        TEST(_Atomic);
        TEST(_Bool);
        TEST(_Complex);
        TEST(_Generic);
        TEST(_Imaginary);
        TEST(_Noreturn);
        TEST(_Static_assert);
        TEST(_Thread_local);

#undef TEST

        ML99_ASSERT_UNEVAL(!ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, restrict, void));
        ML99_ASSERT_UNEVAL(!ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, case, while));
        ML99_ASSERT_UNEVAL(!ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, volatile, _Alignof));
        ML99_ASSERT_UNEVAL(!ML99_IDENT_EQ(ML99_C_KEYWORD_DETECTOR, _Generic, _Alignas));

        ML99_ASSERT(ML99_identEq(v(ML99_C_KEYWORD_DETECTOR), v(_Bool), v(_Bool)));
        ML99_ASSERT(ML99_not(ML99_identEq(v(ML99_C_KEYWORD_DETECTOR), v(_Atomic), v(_Bool))));
    }

    // ML99_UNDERSCORE_DETECTOR
    {
        ML99_ASSERT(ML99_detectIdent(v(ML99_UNDERSCORE_DETECTOR), v(_)));
        ML99_ASSERT(ML99_not(ML99_detectIdent(v(ML99_UNDERSCORE_DETECTOR), v(blah))));
        ML99_ASSERT(ML99_not(ML99_detectIdent(v(ML99_UNDERSCORE_DETECTOR), v())));
    }

    // ML99_LOWERCASE_DETECTOR
    {
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(a), v(a)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(b), v(b)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(c), v(c)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(d), v(d)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(e), v(e)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(f), v(f)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(g), v(g)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(h), v(h)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(i), v(i)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(j), v(j)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(k), v(k)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(l), v(l)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(m), v(m)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(n), v(n)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(o), v(o)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(p), v(p)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(q), v(q)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(r), v(r)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(s), v(s)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(t), v(t)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(u), v(u)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(v), v(v)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(w), v(w)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(x), v(x)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(y), v(y)));
        ML99_ASSERT(ML99_identEq(v(ML99_LOWERCASE_DETECTOR), v(z), v(z)));
    }

    // ML99_UPPERCASE_DETECTOR
    {
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(A), v(A)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(B), v(B)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(C), v(C)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(D), v(D)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(E), v(E)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(F), v(F)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(G), v(G)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(H), v(H)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(I), v(I)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(J), v(J)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(K), v(K)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(L), v(L)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(M), v(M)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(N), v(N)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(O), v(O)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(P), v(P)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(Q), v(Q)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(R), v(R)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(S), v(S)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(T), v(T)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(U), v(U)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(V), v(V)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(W), v(W)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(X), v(X)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(Y), v(Y)));
        ML99_ASSERT(ML99_identEq(v(ML99_UPPERCASE_DETECTOR), v(Z), v(Z)));
    }

    // ML99_DIGIT_DETECTOR
    {
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(0), v(0)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(1), v(1)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(2), v(2)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(3), v(3)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(4), v(4)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(5), v(5)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(6), v(6)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(7), v(7)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(8), v(8)));
        ML99_ASSERT(ML99_identEq(v(ML99_DIGIT_DETECTOR), v(9), v(9)));
    }

    // ML99_IS_LOWERCASE
    {
        ML99_ASSERT_UNEVAL(ML99_IS_LOWERCASE(a));
        ML99_ASSERT_UNEVAL(ML99_IS_LOWERCASE(j));
        ML99_ASSERT_UNEVAL(ML99_IS_LOWERCASE(z));

        ML99_ASSERT_UNEVAL(!ML99_IS_LOWERCASE(8));
        ML99_ASSERT_UNEVAL(!ML99_IS_LOWERCASE(I));
        ML99_ASSERT_UNEVAL(!ML99_IS_LOWERCASE(_));
        ML99_ASSERT_UNEVAL(!ML99_IS_LOWERCASE(abracadabra));

        ML99_ASSERT(ML99_isLowercase(v(z)));
        ML99_ASSERT(ML99_not(ML99_isLowercase(v(8))));
    }

    // ML99_IS_UPPERCASE
    {
        ML99_ASSERT_UNEVAL(ML99_IS_UPPERCASE(A));
        ML99_ASSERT_UNEVAL(ML99_IS_UPPERCASE(J));
        ML99_ASSERT_UNEVAL(ML99_IS_UPPERCASE(Z));

        ML99_ASSERT_UNEVAL(!ML99_IS_UPPERCASE(8));
        ML99_ASSERT_UNEVAL(!ML99_IS_UPPERCASE(i));
        ML99_ASSERT_UNEVAL(!ML99_IS_UPPERCASE(_));
        ML99_ASSERT_UNEVAL(!ML99_IS_UPPERCASE(abracadabra));

        ML99_ASSERT(ML99_isUppercase(v(Z)));
        ML99_ASSERT(ML99_not(ML99_isUppercase(v(8))));
    }

    // ML99_isDigit
    {
        ML99_ASSERT(ML99_isDigit(v(0)));
        ML99_ASSERT(ML99_isDigit(v(2)));
        ML99_ASSERT(ML99_isDigit(v(9)));

        ML99_ASSERT(ML99_not(ML99_isDigit(v(U))));
        ML99_ASSERT(ML99_not(ML99_isDigit(v(i))));
        ML99_ASSERT(ML99_not(ML99_isDigit(v(_))));
        ML99_ASSERT(ML99_not(ML99_isDigit(v(abracadabra))));
    }

    // ML99_IS_DIGIT
    {
        ML99_ASSERT_UNEVAL(ML99_IS_DIGIT(7));
        ML99_ASSERT_UNEVAL(!ML99_IS_DIGIT(k));
    }

    // ML99_isChar
    {
        ML99_ASSERT(ML99_isChar(v(0)));
        ML99_ASSERT(ML99_isChar(v(4)));
        ML99_ASSERT(ML99_isChar(v(8)));

        ML99_ASSERT(ML99_isChar(v(a)));
        ML99_ASSERT(ML99_isChar(v(b)));
        ML99_ASSERT(ML99_isChar(v(c)));

        ML99_ASSERT(ML99_isChar(v(A)));
        ML99_ASSERT(ML99_isChar(v(B)));
        ML99_ASSERT(ML99_isChar(v(C)));

        ML99_ASSERT(ML99_isChar(v(_)));

        ML99_ASSERT(ML99_not(ML99_isChar(v(kk))));
        ML99_ASSERT(ML99_not(ML99_isChar(v(0abc))));
        ML99_ASSERT(ML99_not(ML99_isChar(v(abracadabra))));
    }

    // ML99_IS_CHAR
    {
        ML99_ASSERT_UNEVAL(ML99_IS_CHAR(z));
        ML99_ASSERT_UNEVAL(!ML99_IS_CHAR(xyz));
    }

    // ML99_charLit
    {
        ML99_ASSERT_EQ(ML99_charLit(v(a)), v('a'));
        ML99_ASSERT_EQ(ML99_charLit(v(b)), v('b'));
        ML99_ASSERT_EQ(ML99_charLit(v(c)), v('c'));
        ML99_ASSERT_EQ(ML99_charLit(v(d)), v('d'));
        ML99_ASSERT_EQ(ML99_charLit(v(e)), v('e'));
        ML99_ASSERT_EQ(ML99_charLit(v(f)), v('f'));
        ML99_ASSERT_EQ(ML99_charLit(v(g)), v('g'));
        ML99_ASSERT_EQ(ML99_charLit(v(h)), v('h'));
        ML99_ASSERT_EQ(ML99_charLit(v(i)), v('i'));
        ML99_ASSERT_EQ(ML99_charLit(v(j)), v('j'));
        ML99_ASSERT_EQ(ML99_charLit(v(k)), v('k'));
        ML99_ASSERT_EQ(ML99_charLit(v(l)), v('l'));
        ML99_ASSERT_EQ(ML99_charLit(v(m)), v('m'));
        ML99_ASSERT_EQ(ML99_charLit(v(n)), v('n'));
        ML99_ASSERT_EQ(ML99_charLit(v(o)), v('o'));
        ML99_ASSERT_EQ(ML99_charLit(v(p)), v('p'));
        ML99_ASSERT_EQ(ML99_charLit(v(q)), v('q'));
        ML99_ASSERT_EQ(ML99_charLit(v(r)), v('r'));
        ML99_ASSERT_EQ(ML99_charLit(v(s)), v('s'));
        ML99_ASSERT_EQ(ML99_charLit(v(t)), v('t'));
        ML99_ASSERT_EQ(ML99_charLit(v(u)), v('u'));
        ML99_ASSERT_EQ(ML99_charLit(v(v)), v('v'));
        ML99_ASSERT_EQ(ML99_charLit(v(w)), v('w'));
        ML99_ASSERT_EQ(ML99_charLit(v(x)), v('x'));
        ML99_ASSERT_EQ(ML99_charLit(v(y)), v('y'));
        ML99_ASSERT_EQ(ML99_charLit(v(z)), v('z'));

        ML99_ASSERT_EQ(ML99_charLit(v(A)), v('A'));
        ML99_ASSERT_EQ(ML99_charLit(v(B)), v('B'));
        ML99_ASSERT_EQ(ML99_charLit(v(C)), v('C'));
        ML99_ASSERT_EQ(ML99_charLit(v(D)), v('D'));
        ML99_ASSERT_EQ(ML99_charLit(v(E)), v('E'));
        ML99_ASSERT_EQ(ML99_charLit(v(F)), v('F'));
        ML99_ASSERT_EQ(ML99_charLit(v(G)), v('G'));
        ML99_ASSERT_EQ(ML99_charLit(v(H)), v('H'));
        ML99_ASSERT_EQ(ML99_charLit(v(I)), v('I'));
        ML99_ASSERT_EQ(ML99_charLit(v(J)), v('J'));
        ML99_ASSERT_EQ(ML99_charLit(v(K)), v('K'));
        ML99_ASSERT_EQ(ML99_charLit(v(L)), v('L'));
        ML99_ASSERT_EQ(ML99_charLit(v(M)), v('M'));
        ML99_ASSERT_EQ(ML99_charLit(v(N)), v('N'));
        ML99_ASSERT_EQ(ML99_charLit(v(O)), v('O'));
        ML99_ASSERT_EQ(ML99_charLit(v(P)), v('P'));
        ML99_ASSERT_EQ(ML99_charLit(v(Q)), v('Q'));
        ML99_ASSERT_EQ(ML99_charLit(v(R)), v('R'));
        ML99_ASSERT_EQ(ML99_charLit(v(S)), v('S'));
        ML99_ASSERT_EQ(ML99_charLit(v(T)), v('T'));
        ML99_ASSERT_EQ(ML99_charLit(v(U)), v('U'));
        ML99_ASSERT_EQ(ML99_charLit(v(V)), v('V'));
        ML99_ASSERT_EQ(ML99_charLit(v(W)), v('W'));
        ML99_ASSERT_EQ(ML99_charLit(v(X)), v('X'));
        ML99_ASSERT_EQ(ML99_charLit(v(Y)), v('Y'));
        ML99_ASSERT_EQ(ML99_charLit(v(Z)), v('Z'));

        ML99_ASSERT_EQ(ML99_charLit(v(0)), v('0'));
        ML99_ASSERT_EQ(ML99_charLit(v(1)), v('1'));
        ML99_ASSERT_EQ(ML99_charLit(v(2)), v('2'));
        ML99_ASSERT_EQ(ML99_charLit(v(3)), v('3'));
        ML99_ASSERT_EQ(ML99_charLit(v(4)), v('4'));
        ML99_ASSERT_EQ(ML99_charLit(v(5)), v('5'));
        ML99_ASSERT_EQ(ML99_charLit(v(6)), v('6'));
        ML99_ASSERT_EQ(ML99_charLit(v(7)), v('7'));
        ML99_ASSERT_EQ(ML99_charLit(v(8)), v('8'));
        ML99_ASSERT_EQ(ML99_charLit(v(9)), v('9'));

        ML99_ASSERT_EQ(ML99_charLit(v(_)), v('_'));
    }

    // ML99_CHAR_LIT
    {
        ML99_ASSERT_UNEVAL(ML99_CHAR_LIT(r) == 'r');
        ML99_ASSERT_UNEVAL(ML99_CHAR_LIT(8) == '8');
        ML99_ASSERT_UNEVAL(ML99_CHAR_LIT(_) == '_');
    }

#define FST(...)        FST_AUX(__VA_ARGS__)
#define FST_AUX(x, ...) x

    // ML99_LOWERCASE_CHARS, ML99_UPPERCASE_CHARS, ML99_DIGITS
    {
        ML99_ASSERT_UNEVAL(ML99_CHAR_EQ(a, FST(ML99_LOWERCASE_CHARS(~, ~, ~))));
        ML99_ASSERT_UNEVAL(ML99_CHAR_EQ(A, FST(ML99_UPPERCASE_CHARS(~, ~, ~))));
        ML99_ASSERT_UNEVAL(FST(ML99_DIGITS(~, ~, ~)) == 0);
    }

#undef FST
#undef FST_AUX
}
