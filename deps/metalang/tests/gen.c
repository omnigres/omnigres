#include <metalang99/assert.h>
#include <metalang99/gen.h>
#include <metalang99/ident.h>
#include <metalang99/list.h>
#include <metalang99/tuple.h>

#include <assert.h>

// clang-format off
static void
    test_indexed_params ML99_EVAL(ML99_indexedParams(ML99_list(v(int, long long, const char *))))
// clang-format on
{
    int i = _0;
    long long ll = _1;
    const char *str = _2;

    (void)i;
    (void)ll;
    (void)str;
}

static int test_fn_ptr(const char *str, long long x) {
    (void)str;
    (void)x;
    return 123;
}

int main(void) {

    // ML99_braced
    { struct TestBraced ML99_EVAL(ML99_braced(v(int a, b, c;))); }

    // ML99_semicoloned
    {
        ML99_EVAL(ML99_semicoloned(v(struct TestSemicoloned { int a, b, c; })));
    }

    // ML99_assign(Stmt)
    {
        int x = 0;

        ML99_EVAL(ML99_assign(v(x), v(5)));
        assert(5 == x);

        ML99_EVAL(ML99_assignStmt(v(x), v(7)))
        assert(7 == x);
    }

    // ML99_assignInitializerList(Stmt)
    {
        typedef struct {
            int x, y;
        } Point;

        ML99_EVAL(ML99_assignInitializerList(v(Point p1), v(.x = 2, .y = 3)));
        assert(2 == p1.x);
        assert(3 == p1.y);

        ML99_EVAL(ML99_assignInitializerListStmt(v(Point p2), v(.x = 5, .y = 7)))
        assert(5 == p2.x);
        assert(7 == p2.y);
    }

#define F(a, b, c) ML99_ASSERT_UNEVAL(a == 1 && b == 2 && c == 3)

    // ML99_invoke(Stmt)
    {
        ML99_EVAL(ML99_invoke(v(F), v(1, 2, 3)));
        ML99_EVAL(ML99_invokeStmt(v(F), v(1, 2, 3)))
    }

#undef F

    // ML99_prefixedBlock
    {

        ML99_EVAL(ML99_prefixedBlock(v(if (1)), v(goto end_prefixed_block;)))

        // Unreachable code.
        assert(0);

    end_prefixed_block:;
    }

    // ML99_typedef
    {

        ML99_EVAL(ML99_typedef(v(Point), v(struct { int x, y; })));

        Point point = {5, 7};
        point.x = 1;
        point.y = 2;

        (void)point;
    }

    // ML99_struct
    {

        ML99_EVAL(ML99_struct(v(Point), v(int x, y;)));

        struct Point point = {5, 7};
        point.x = 1;
        point.y = 2;

        (void)point;
    }

    // ML99_anonStruct
    {
        typedef ML99_EVAL(ML99_anonStruct(v(int x, y;)))
        Point;

        Point point = {5, 7};
        point.x = 1;
        point.y = 2;

        (void)point;
    }

    // ML99_union
    {
        ML99_EVAL(ML99_union(v(Point), v(int x, y;)));

        union Point point;
        point.x = 1;
        point.y = 2;

        (void)point;
    }

    // ML99_anonUnion
    {
        typedef ML99_EVAL(ML99_anonUnion(v(int x, y;)))
        Point;

        Point point;
        point.x = 1;
        point.y = 2;

        (void)point;
    }

    // ML99_enum
    {
        ML99_EVAL(ML99_enum(v(MyEnum), v(Foo, Bar)));

        enum MyEnum foo = Foo, bar = Bar;
        (void)foo;
        (void)bar;
    }

    // ML99_anonEnum
    {
        typedef ML99_EVAL(ML99_anonEnum(v(Foo, Bar)))
        MyEnum;

        MyEnum foo = Foo, bar = Bar;
        (void)foo;
        (void)bar;
    }

    // ML99_fnPtr(Stmt)
    {
        {
            ML99_EVAL(ML99_fnPtr(v(int), v(ptr), v(const char *str), v(long long x)))
            = test_fn_ptr;
            assert(test_fn_ptr == ptr);
        }

        {
            ML99_EVAL(ML99_fnPtrStmt(v(int), v(ptr), v(const char *str), v(long long x)))
            ptr = test_fn_ptr;
            (void)ptr;
        }
    }

#define CHECK_EXPAND(args) CHECK(args)

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 0 && y == 1 && z == 2)
#define F_IMPL(x)         v(, x)
#define F_ARITY           1

    // ML99_repeat
    { CHECK_EXPAND(ML99_EVAL(ML99_repeat(v(3), v(F)))); }

#undef CHECK
#undef F_IMPL
#undef F_ARITY

#define CHECK(_, x, y, z) ML99_ASSERT_UNEVAL(x == 5 && y == 5 && z == 5)

    // ML99_times
    { CHECK_EXPAND(ML99_EVAL(ML99_times(v(3), v(, 5)))); }

#undef CHECK

#undef CHECK_EXPAND

    // ML99_indexedParams
    {
        ML99_ASSERT_UNEVAL(ML99_IDENT_EQ(
            ML99_C_KEYWORD_DETECTOR,
            void,
            ML99_EVAL(ML99_untuple(ML99_indexedParams(ML99_nil())))));

        (void)test_indexed_params;
    }

    // ML99_indexedFields
    {
        ML99_ASSERT_EMPTY(ML99_indexedFields(ML99_nil()));

        struct {
            ML99_EVAL(ML99_indexedFields(ML99_list(v(int, long long, const char *))))
        } data = {0};

        int i = data._0;
        long long ll = data._1;
        const char *str = data._2;

        (void)i;
        (void)ll;
        (void)str;
    }

    // clang-format off

    // ML99_indexedInitializerList
    {
        // When N=0.
        {
            const struct {
                int _0;
                long long _1;
                const char *_2;
            } test = ML99_EVAL(ML99_indexedInitializerList(v(0)));

        assert(0 == test._0);
        assert(0 == test._1);
        assert(0 == test._2);
        }

        // When N>0.
        {
            int _0 = 123;
            long long _1 = 149494456;
            const char *_2 = "abc";

            struct {
                int i;
                long long ll;
                const char *str;
            } data = ML99_EVAL(ML99_indexedInitializerList(v(3)));

            (void)data;
        }
    }

    // ML99_indexedArgs
    {
        ML99_ASSERT_EMPTY(ML99_indexedArgs(v(0)));

        int _0 = 123;
        long long _1 = 149494456;
        const char *_2 = "abc";

        const struct {
            int i;
            long long ll;
            const char *str;
        } test = { ML99_EVAL(ML99_indexedArgs(v(3))) };

        assert(test.i == _0);
        assert(test.ll == _1);
        assert(test.str == _2);
    }

// clang-format on

return 0;
}
