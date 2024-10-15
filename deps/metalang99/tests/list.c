#include <metalang99/assert.h>
#include <metalang99/list.h>
#include <metalang99/maybe.h>
#include <metalang99/nat.h>
#include <metalang99/tuple.h>
#include <metalang99/util.h>

int main(void) {

#define CMP_NATURALS(lhs, rhs) ML99_listEq(v(ML99_natEq), lhs, rhs)

    // ML99_list, ML99_cons, ML99_nil
    {
        ML99_ASSERT(CMP_NATURALS(ML99_nil(v(~, ~, ~)), ML99_nil()));

        ML99_ASSERT(ML99_listEq(
            v(ML99_natEq),
            ML99_list(v(1, 2, 3, 4, 5, 6, 7)),
            ML99_cons(
                v(1),
                ML99_cons(
                    v(2),
                    ML99_cons(
                        v(3),
                        ML99_cons(
                            v(4),
                            ML99_cons(v(5), ML99_cons(v(6), ML99_cons(v(7), ML99_nil())))))))));
    }

#define F_IMPL(x, y) ML99_add(v(x), v(y))
#define F_ARITY      1

    // ML99_listFromTuples
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listFromTuples(v(F), v((1, 2))), ML99_list(v(3))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listFromTuples(v(F), v((1, 2), (3, 4), (5, 6))),
            ML99_list(v(3, 7, 11))));
    }

#undef F_IMPL
#undef F_ARITY

    // ML99_listFromSeq
    {
        ML99_ASSERT(ML99_isNil(ML99_listFromSeq(ML99_empty())));
        ML99_ASSERT(CMP_NATURALS(ML99_listFromSeq(v((1))), ML99_list(v(1))));
        ML99_ASSERT(CMP_NATURALS(ML99_listFromSeq(v((1)(2))), ML99_list(v(1, 2))));
        ML99_ASSERT(CMP_NATURALS(ML99_listFromSeq(v((1)(2)(3))), ML99_list(v(1, 2, 3))));
    }

    // ML99_listHead
    {
        ML99_ASSERT_EQ(ML99_listHead(ML99_list(v(1))), v(1));
        ML99_ASSERT_EQ(ML99_listHead(ML99_list(v(1, 2))), v(1));
        ML99_ASSERT_EQ(ML99_listHead(ML99_list(v(1, 2, 3))), v(1));
    }

    // ML99_listTail
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listTail(ML99_list(v(1))), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(ML99_listTail(ML99_list(v(1, 2))), ML99_list(v(2))));
        ML99_ASSERT(CMP_NATURALS(ML99_listTail(ML99_list(v(1, 2, 3))), ML99_list(v(2, 3))));
    }

    // ML99_listLast
    {
        ML99_ASSERT_EQ(ML99_listLast(ML99_list(v(1))), v(1));
        ML99_ASSERT_EQ(ML99_listLast(ML99_list(v(1, 2))), v(2));
        ML99_ASSERT_EQ(ML99_listLast(ML99_list(v(1, 2, 3))), v(3));
    }

    // ML99_listInit
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listInit(ML99_list(v(1))), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(ML99_listInit(ML99_list(v(1, 2))), ML99_list(v(1))));
        ML99_ASSERT(CMP_NATURALS(ML99_listInit(ML99_list(v(1, 2, 3))), ML99_list(v(1, 2))));
    }

    // ML99_listLen
    {
        ML99_ASSERT_EQ(ML99_listLen(ML99_nil()), v(0));
        ML99_ASSERT_EQ(ML99_listLen(ML99_list(v(123))), v(1));
        ML99_ASSERT_EQ(ML99_listLen(ML99_list(v(123, 222))), v(2));
        ML99_ASSERT_EQ(ML99_listLen(ML99_list(v(123, 222, 18))), v(3));
    }

    // ML99_listAppend
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listAppend(ML99_nil(), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listAppend(ML99_nil(), ML99_list(v(1, 2, 3))),
            ML99_list(v(1, 2, 3))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listAppend(ML99_list(v(1, 2, 3)), ML99_nil()),
            ML99_list(v(1, 2, 3))));

        ML99_ASSERT(CMP_NATURALS(
            ML99_listAppend(ML99_list(v(1, 2, 3)), ML99_list(v(4, 5, 6))),
            ML99_list(v(1, 2, 3, 4, 5, 6))));
    }

    // ML99_listAppendItem
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listAppendItem(v(123), ML99_nil()), ML99_list(v(123))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listAppendItem(v(222), ML99_list(v(1, 2, 3))),
            ML99_list(v(1, 2, 3, 222))));
    }

    // ML99_listReverse
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listReverse(ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(ML99_listReverse(ML99_list(v(1, 2, 3))), ML99_list(v(3, 2, 1))));
    }

    // ML99_listContains
    {
        ML99_ASSERT(ML99_not(ML99_listContains(v(ML99_natEq), v(1), ML99_nil())));
        ML99_ASSERT(ML99_listContains(v(ML99_natEq), v(1), ML99_list(v(1, 2, 3))));
        ML99_ASSERT(ML99_listContains(v(ML99_natEq), v(2), ML99_list(v(1, 2, 3))));
        ML99_ASSERT(ML99_listContains(v(ML99_natEq), v(3), ML99_list(v(1, 2, 3))));
        ML99_ASSERT(ML99_not(ML99_listContains(v(ML99_natEq), v(187), ML99_list(v(1, 2, 3)))));
    }

    // ML99_listUnwrap
    {
        ML99_ASSERT_EMPTY(ML99_listUnwrap(ML99_nil()));
        ML99_ASSERT_EQ(ML99_listUnwrap(ML99_list(v(18, +, 3, +, 6))), v(18 + 3 + 6));
    }

    // ML99_LIST_EVAL
    {
        ML99_ASSERT_EMPTY_UNEVAL(ML99_LIST_EVAL(ML99_nil()));
        ML99_ASSERT_UNEVAL(ML99_LIST_EVAL(ML99_list(v(19, +, 6))) == 19 + 6);
    }

#define CHECK(a, b, c)     ML99_ASSERT_UNEVAL(a == 1 && b == 2 && c == 3)
#define CHECK_EXPAND(args) CHECK(args)

    // ML99_listUnwrapCommaSep
    {
        ML99_ASSERT_EMPTY(ML99_listUnwrapCommaSep(ML99_nil()));
        CHECK_EXPAND(ML99_EVAL(ML99_listUnwrapCommaSep(ML99_list(v(1, 2, 3)))));
    }

    // ML99_LIST_EVAL_COMMA_SEP
    {
        ML99_ASSERT_EMPTY_UNEVAL(ML99_LIST_EVAL_COMMA_SEP(ML99_nil()));
        CHECK_EXPAND(ML99_EVAL(v(ML99_LIST_EVAL_COMMA_SEP(ML99_list(v(1, 2, 3))))));
    }

#undef CHECK
#undef CHECK_EXPAND

    // ML99_isNil
    {
        ML99_ASSERT(ML99_isNil(ML99_nil()));
        ML99_ASSERT(ML99_not(ML99_isNil(ML99_list(v(123)))));
        ML99_ASSERT(ML99_not(ML99_isNil(ML99_list(v(8, 214, 10, 0, 122)))));
    }

    // ML99_IS_NIL
    {
        ML99_ASSERT_UNEVAL(ML99_IS_NIL(ML99_NIL()));
        ML99_ASSERT_UNEVAL(ML99_NOT(ML99_IS_NIL(ML99_CONS(123, ML99_NIL()))));
        ML99_ASSERT_UNEVAL(ML99_NOT(ML99_IS_NIL(ML99_EVAL(ML99_list(v(8, 214, 10, 0, 122))))));
    }

    // ML99_isCons
    {
        ML99_ASSERT(ML99_not(ML99_isCons(ML99_nil())));
        ML99_ASSERT(ML99_isCons(ML99_list(v(123))));
        ML99_ASSERT(ML99_isCons(ML99_list(v(8, 214, 10, 0, 122))));
    }

    // ML99_IS_CONS
    {
        ML99_ASSERT_UNEVAL(ML99_NOT(ML99_IS_CONS(ML99_NIL())));
        ML99_ASSERT_UNEVAL(ML99_IS_CONS(ML99_CONS(123, ML99_NIL())));
        ML99_ASSERT_UNEVAL(ML99_IS_CONS(ML99_EVAL(ML99_list(v(8, 214, 10, 0, 122)))));
    }

    // ML99_listGet
    {
        ML99_ASSERT_EQ(ML99_listGet(v(0), ML99_list(v(123, 222))), v(123));
        ML99_ASSERT_EQ(ML99_listGet(v(1), ML99_list(v(123, 222))), v(222));
    }

    // ML99_listTake
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listTake(v(1), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(ML99_listTake(v(200), ML99_nil()), ML99_nil()));

        ML99_ASSERT(CMP_NATURALS(ML99_listTake(v(1), ML99_list(v(1, 2, 3))), ML99_list(v(1))));
        ML99_ASSERT(CMP_NATURALS(ML99_listTake(v(2), ML99_list(v(1, 2, 3))), ML99_list(v(1, 2))));
        ML99_ASSERT(
            CMP_NATURALS(ML99_listTake(v(3), ML99_list(v(1, 2, 3))), ML99_list(v(1, 2, 3))));
    }

    // ML99_listTakeWhile
    {
        ML99_ASSERT(CMP_NATURALS(
            ML99_listTakeWhile(ML99_appl(v(ML99_lesser), v(5)), ML99_nil()),
            ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listTakeWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(7))),
            ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listTakeWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(1, 9, 7))),
            ML99_list(v(1))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listTakeWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(4, 9, 2, 3))),
            ML99_list(v(4))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listTakeWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(2, 4, 7, 9, 28))),
            ML99_list(v(2, 4))));
    }

    // ML99_listDrop
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listDrop(v(1), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(ML99_listDrop(v(200), ML99_nil()), ML99_nil()));

        ML99_ASSERT(CMP_NATURALS(ML99_listDrop(v(1), ML99_list(v(1, 2, 3))), ML99_list(v(2, 3))));
        ML99_ASSERT(CMP_NATURALS(ML99_listDrop(v(2), ML99_list(v(1, 2, 3))), ML99_list(v(3))));
        ML99_ASSERT(CMP_NATURALS(ML99_listDrop(v(3), ML99_list(v(1, 2, 3))), ML99_nil()));
    }

    // ML99_listDropWhile
    {
        ML99_ASSERT(CMP_NATURALS(
            ML99_listDropWhile(ML99_appl(v(ML99_lesser), v(5)), ML99_nil()),
            ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listDropWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(7))),
            ML99_list(v(7))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listDropWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(1, 9, 7))),
            ML99_list(v(9, 7))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listDropWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(4, 9, 2, 3))),
            ML99_list(v(9, 2, 3))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listDropWhile(ML99_appl(v(ML99_greater), v(5)), ML99_list(v(2, 4, 7, 9, 28))),
            ML99_list(v(7, 9, 28))));
    }

#define EQ_IMPL(x, y)                                                                              \
    v(ML99_AND(                                                                                    \
        ML99_NAT_EQ(ML99_TUPLE_GET(0)(x), ML99_TUPLE_GET(0)(y)),                                   \
        ML99_NAT_EQ(ML99_TUPLE_GET(1)(x), ML99_TUPLE_GET(1)(y))))
#define EQ_ARITY 2

    // ML99_listZip
    {
        ML99_ASSERT(ML99_listEq(v(EQ), ML99_listZip(ML99_nil(), ML99_nil()), ML99_nil()));
        ML99_ASSERT(
            ML99_listEq(v(EQ), ML99_listZip(ML99_list(v(1, 2, 3)), ML99_nil()), ML99_nil()));
        ML99_ASSERT(
            ML99_listEq(v(EQ), ML99_listZip(ML99_nil(), ML99_list(v(1, 2, 3))), ML99_nil()));

        ML99_ASSERT(ML99_listEq(
            v(EQ),
            ML99_listZip(ML99_list(v(1, 2, 3)), ML99_list(v(4, 5, 6))),
            ML99_list(ML99_tuple(v(1, 4)), ML99_tuple(v(2, 5)), ML99_tuple(v(3, 6)))));

        ML99_ASSERT(ML99_listEq(
            v(EQ),
            ML99_listZip(ML99_list(v(1, 2, 3)), ML99_list(v(4, 5))),
            ML99_list(ML99_tuple(v(1, 4)), ML99_tuple(v(2, 5)))));

        ML99_ASSERT(ML99_listEq(
            v(EQ),
            ML99_listZip(ML99_list(v(1, 2)), ML99_list(v(4, 5, 6))),
            ML99_list(ML99_tuple(v(1, 4)), ML99_tuple(v(2, 5)))));
    }

#undef EQ_IMPL
#undef EQ_ARITY

    // ML99_listUnzip & ML99_listZip
    {
#define UNZIPPED ML99_listUnzip(ML99_nil())

        ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(0)(UNZIPPED), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(1)(UNZIPPED), ML99_nil()));

#undef UNZIPPED

#define UNZIPPED ML99_listUnzip(ML99_listZip(ML99_list(v(1, 2)), ML99_list(v(4, 5))))

        ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(0)(UNZIPPED), ML99_list(v(1, 2))));
        ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(1)(UNZIPPED), ML99_list(v(4, 5))));

#undef UNZIPPED
    }

    // ML99_listEq
    {
        ML99_ASSERT(CMP_NATURALS(ML99_nil(), ML99_nil()));
        ML99_ASSERT(ML99_not(CMP_NATURALS(ML99_nil(), ML99_list(v(25, 88, 1)))));
        ML99_ASSERT(ML99_not(CMP_NATURALS(ML99_list(v(25, 88, 1)), ML99_nil())));

        ML99_ASSERT(CMP_NATURALS(ML99_list(v(1, 2, 3)), ML99_list(v(1, 2, 3))));
        ML99_ASSERT(ML99_not(CMP_NATURALS(ML99_list(v(1, 2, 3)), ML99_list(v(1)))));
        ML99_ASSERT(ML99_not(CMP_NATURALS(ML99_list(v(1)), ML99_list(v(1, 2, 3)))));
        ML99_ASSERT(ML99_not(CMP_NATURALS(ML99_list(v(0, 5, 6, 6)), ML99_list(v(6, 7)))));
    }

    // ML99_listAppl
    {
        ML99_ASSERT_EQ(ML99_call(ML99_listAppl(v(ML99_add), ML99_nil()), v(6, 9)), v(6 + 9));
        ML99_ASSERT_EQ(ML99_appl(ML99_listAppl(v(ML99_add), ML99_list(v(6))), v(9)), v(6 + 9));
        ML99_ASSERT_EQ(ML99_listAppl(v(ML99_add), ML99_list(v(6, 9))), v(6 + 9));
    }

    // ML99_listPrependToAll
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listPrependToAll(v(+), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listPrependToAll(v(111), ML99_list(v(5, 9, 22))),
            ML99_list(v(111, 5, 111, 9, 111, 22))));
    }

    // ML99_listIntersperse
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listIntersperse(v(+), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listIntersperse(v(111), ML99_list(v(5, 9, 22))),
            ML99_list(v(5, 111, 9, 111, 22))));
    }

#define ABCDEFG ML99_TRUE()

    // ML99_listFoldr
    {
        ML99_ASSERT_EQ(ML99_listFoldr(v(ML99_cat), v(7), ML99_nil()), v(7));
        ML99_ASSERT(
            ML99_listFoldr(ML99_appl(v(ML99_flip), v(ML99_cat)), v(A), ML99_list(v(G, DEF, BC))));
    }

    // ML99_listFoldl
    {
        ML99_ASSERT_EQ(ML99_listFoldl(v(ML99_cat), v(7), ML99_nil()), v(7));
        ML99_ASSERT(ML99_listFoldl(v(ML99_cat), v(A), ML99_list(v(BC, DEF, G))));
    }

    // ML99_listFoldl1
    { ML99_ASSERT(ML99_listFoldl1(v(ML99_cat), ML99_list(v(AB, CDEF, G)))); }

#undef ABCDEFG

    // ML99_listMap
    {
        ML99_ASSERT(
            CMP_NATURALS(ML99_listMap(ML99_appl(v(ML99_add), v(3)), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listMap(ML99_appl(v(ML99_add), v(3)), ML99_list(v(1, 2, 3))),
            ML99_list(v(4, 5, 6))));
    }

#define A0 19
#define B1 6
#define C2 11

    // ML99_listMapI
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listMapI(v(ML99_cat), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listMapI(v(ML99_cat), ML99_list(v(A, B, C))),
            ML99_list(v(19, 6, 11))));
    }

#undef A0
#undef B1
#undef C2

#define FOO_x
#define FOO_y
#define FOO_z

    // ML99_listMapInPlace
    {
        ML99_ASSERT_EMPTY(ML99_listMapInPlace(v(NonExistingF), ML99_nil()));
        ML99_EVAL(ML99_listMapInPlace(ML99_appl(v(ML99_cat), v(FOO_)), ML99_list(v(x, y, z))))
    }

#undef FOO_x
#undef FOO_y
#undef FOO_z

#define FOO_x0
#define FOO_y1
#define FOO_z2

#define MY_CAT_IMPL(x, i) v(FOO_##x##i)
#define MY_CAT_ARITY      2

    // ML99_listMapInPlaceI
    {
        ML99_ASSERT_EMPTY(ML99_listMapInPlaceI(v(NonExistingF), ML99_nil()));
        ML99_EVAL(ML99_listMapInPlaceI(v(MY_CAT), ML99_list(v(x, y, z))))
    }

#undef MY_CAT_IMPL
#undef MY_CAT_ARITY

#undef FOO_x
#undef FOO_y
#undef FOO_z

    // ML99_listFor
    {
        ML99_ASSERT(
            CMP_NATURALS(ML99_listFor(ML99_nil(), ML99_appl(v(ML99_add), v(3))), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listFor(ML99_list(v(1, 2, 3)), ML99_appl(v(ML99_add), v(3))),
            ML99_list(v(4, 5, 6))));
    }

    // ML99_listMapInitLast
    {
        ML99_ASSERT(CMP_NATURALS(
            ML99_listMapInitLast(
                ML99_appl(v(ML99_add), v(3)),
                ML99_appl(v(ML99_add), v(19)),
                ML99_list(v(4))),
            ML99_list(v(23))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listMapInitLast(
                ML99_appl(v(ML99_add), v(3)),
                ML99_appl(v(ML99_add), v(7)),
                ML99_list(v(1, 2, 3))),
            ML99_list(v(4, 5, 10))));
    }

    // ML99_listForInitLast
    {
        ML99_ASSERT(CMP_NATURALS(
            ML99_listForInitLast(
                ML99_list(v(4)),
                ML99_appl(v(ML99_add), v(3)),
                ML99_appl(v(ML99_add), v(19))),
            ML99_list(v(23))));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listForInitLast(
                ML99_list(v(1, 2, 3)),
                ML99_appl(v(ML99_add), v(3)),
                ML99_appl(v(ML99_add), v(7))),
            ML99_list(v(4, 5, 10))));
    }

    // ML99_listFilter
    {
        ML99_ASSERT(
            CMP_NATURALS(ML99_listFilter(ML99_appl(v(ML99_add), v(3)), ML99_nil()), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(
            ML99_listFilter(ML99_appl(v(ML99_lesser), v(3)), ML99_list(v(14, 0, 1, 7, 2, 65, 3))),
            ML99_list(v(14, 7, 65))));
    }

    // ML99_listFilterMap
    {
#define LIST ML99_list(v(5, 7, 3, 12))

#define F_IMPL(x) ML99_if(ML99_lesserEq(v(x), v(5)), ML99_just(v(x)), ML99_nothing())
#define F_ARITY   1

        ML99_ASSERT(CMP_NATURALS(ML99_listFilterMap(v(F), LIST), ML99_list(v(5, 3))));

#undef F_IMPL
#undef F_ARITY

#undef LIST
    }

    // ML99_listReplicate
    {
        ML99_ASSERT(CMP_NATURALS(ML99_listReplicate(v(0), v(~)), ML99_nil()));
        ML99_ASSERT(CMP_NATURALS(ML99_listReplicate(v(3), v(7)), ML99_list(v(7, 7, 7))));
    }

    // ML99_listPartition
    {

        // Partitioning ML99_nil()
        {
#define PARTITIONED ML99_listPartition(ML99_appl(v(ML99_greater), v(10)), ML99_nil())

            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(0)(PARTITIONED), ML99_nil()));
            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(1)(PARTITIONED), ML99_nil()));

#undef PARTITIONED
        }

        // Only the second list contains items
        {
#define PARTITIONED ML99_listPartition(ML99_appl(v(ML99_greater), v(10)), ML99_list(v(11, 12, 13)))

            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(0)(PARTITIONED), ML99_nil()));
            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(1)(PARTITIONED), ML99_list(v(11, 12, 13))));

#undef PARTITIONED
        }

        // Only the first list contains items
        {
#define PARTITIONED ML99_listPartition(ML99_appl(v(ML99_greater), v(10)), ML99_list(v(4, 7)))

            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(0)(PARTITIONED), ML99_list(v(4, 7))));
            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(1)(PARTITIONED), ML99_nil()));

#undef PARTITIONED
        }

        // Both lists contain items
        {
#define PARTITIONED                                                                                \
    ML99_listPartition(ML99_appl(v(ML99_greater), v(10)), ML99_list(v(11, 4, 12, 13, 7)))

            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(0)(PARTITIONED), ML99_list(v(4, 7))));
            ML99_ASSERT(CMP_NATURALS(ML99_tupleGet(1)(PARTITIONED), ML99_list(v(11, 12, 13))));

#undef PARTITIONED
        }
    }
}
