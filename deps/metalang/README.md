# Metalang99

[![CI](https://github.com/Hirrolot/metalang99/workflows/C/C++%20CI/badge.svg)](https://github.com/Hirrolot/metalang99/actions)
[![docs](https://img.shields.io/badge/docs-readthedocs.io-blue)](https://metalang99.readthedocs.io/en/latest/)
[![book](https://img.shields.io/badge/book-gitbook.io-pink)](https://hirrolot.gitbook.io/metalang99/)
[![spec](https://img.shields.io/badge/spec-PDF-green)](https://github.com/Hirrolot/metalang99/blob/master/spec/spec.pdf)
[![mailing list](https://img.shields.io/badge/mailing%20list-lists.sr.ht-orange)](https://lists.sr.ht/~hirrolot/metalang99)

> The dark side of the force is a pathway to many abilities, some considered to be unnatural.<br>&emsp; &emsp; <b>-- Darth Sidious</b>

Based on [`examples/demo.c`](examples/demo.c):

<table>
<tr><td><b>Compile-time list manipulation</b></td></tr>

<tr>
<td>

```c
// 3, 3, 3, 3, 3
static int five_threes[] = {
    ML99_LIST_EVAL_COMMA_SEP(ML99_listReplicate(v(5), v(3))),
};

// 5, 4, 3, 2, 1
static int from_5_to_1[] = {
    ML99_LIST_EVAL_COMMA_SEP(ML99_listReverse(ML99_list(v(1, 2, 3, 4, 5)))),
};

// 9, 2, 5
static int lesser_than_10[] = {
    ML99_LIST_EVAL_COMMA_SEP(
        ML99_listFilter(ML99_appl(v(ML99_greater), v(10)), ML99_list(v(9, 2, 11, 13, 5)))),
};
```

</td>
</tr>
</table>

<table>
<tr><td><b>Macro recursion</b></td></tr>

<tr>
<td>

```c
#define factorial(n)          ML99_natMatch(n, v(factorial_))
#define factorial_Z_IMPL(...) v(1)
#define factorial_S_IMPL(n)   ML99_mul(ML99_inc(v(n)), factorial(v(n)))

ML99_ASSERT_EQ(factorial(v(4)), v(24));
```

</td>
</tr>
</table>

<table>
<tr><td><b>Overloading on a number of arguments</b></td></tr>

<tr>
<td>

```c
typedef struct {
    double width, height;
} Rect;

#define Rect_new(...) ML99_OVERLOAD(Rect_new_, __VA_ARGS__)
#define Rect_new_1(x)                                                                              \
    { x, x }
#define Rect_new_2(x, y)                                                                           \
    { x, y }

static Rect _7x8 = Rect_new(7, 8), _10x10 = Rect_new(10);

// ... and more!

int main(void) {
    // Yeah. All is done at compile time.
}
```

</td>
</tr>
</table>

(Hint: `v(something)` evaluates to `something`.)

Metalang99 is a firm foundation for writing reliable and maintainable metaprograms in pure C99. It is implemented as an interpreted FP language atop of preprocessor macros: just `#include <metalang99.h>` and you are ready to go. Metalang99 features algebraic data types, pattern matching, recursion, currying, and collections; in addition, it provides means for compile-time error reporting and debugging. With our [built-in syntax checker], macro errors should be perfectly comprehensible, enabling you for convenient development.

[built-in syntax checker]: #q-what-about-compile-time-errors

Currently, Metalang99 is used at [OpenIPC] as an indirect dependency of [Datatype99] and [Interface99]; this includes an [RTSP 1.0 implementation] along with ~50k lines of private code.

[OpenIPC]: https://openipc.org/
[RTSP 1.0 implementation]: https://github.com/OpenIPC/smolrtsp/

[Datatype99]: https://github.com/Hirrolot/Datatype99
[Interface99]: https://github.com/Hirrolot/Interface99

## Motivation

Macros facilitate code re-use, macros are the building material that lets you shape the language to suit the problem being solved, leading to more clean and concise code. However, metaprogramming in C is utterly castrated: we cannot even operate with control flow, integers, unbounded sequences, and compound data structures, thereby throwing a lot of hypothetically useful metaprograms out of scope.

To solve the problem, I have implemented Metalang99. Having its functionality at our disposal, it becomes possible to develop even fairly non-trivial metaprograms, such as [Datatype99]:

```c
#include <datatype99.h>

datatype(
    BinaryTree,
    (Leaf, int),
    (Node, BinaryTree *, int, BinaryTree *)
);

int sum(const BinaryTree *tree) {
    match(*tree) {
        of(Leaf, x) return *x;
        of(Node, lhs, x, rhs) return sum(*lhs) + *x + sum(*rhs);
    }

    return -1;
}
```

Or [Interface99]:

```c
#include <interface99.h>

#include <stdio.h>

#define Shape_IFACE                      \
    vfunc( int, perim, const VSelf)      \
    vfunc(void, scale, VSelf, int factor)

interface(Shape);

typedef struct {
    int a, b;
} Rectangle;

int  Rectangle_perim(const VSelf) { /* ... */ }
void Rectangle_scale(VSelf, int factor) { /* ... */ }

impl(Shape, Rectangle);

typedef struct {
    int a, b, c;
} Triangle;

int  Triangle_perim(const VSelf) { /* ... */ }
void Triangle_scale(VSelf, int factor) { /* ... */ }

impl(Shape, Triangle);

void test(Shape shape) {
    printf("perim = %d\n", VCALL(shape, perim));
    VCALL(shape, scale, 5);
    printf("perim = %d\n", VCALL(shape, perim));
}
```

Unlike the vague techniques, such as [tagged unions] or [virtual method tables], the above metaprograms leverage type safety, syntax conciseness, and maintain the exact memory layout of generated code.

Looks interesting? Check out the [motivational post] for more information.

[tagged unions]: https://en.wikipedia.org/wiki/Tagged_union
[virtual method tables]: https://en.wikipedia.org/wiki/Virtual_method_table
[motivational post]: https://hirrolot.github.io/posts/macros-on-steroids-or-how-can-pure-c-benefit-from-metaprogramming.html

## Getting started

Metalang99 is just a set of header files and nothing else; therefore, the only thing you need to tell your compiler is to add `metalang99/include` to include directories.

If you use CMake, the recommended way is [`FetchContent`]:

[`FetchContent`]: https://cmake.org/cmake/help/latest/module/FetchContent.html

```cmake
include(FetchContent)

FetchContent_Declare(
    metalang99
    URL https://github.com/Hirrolot/metalang99/archive/refs/tags/v1.2.3.tar.gz # v1.2.3
)

FetchContent_MakeAvailable(metalang99)

target_link_libraries(MyProject metalang99)

# Metalang99 relies on heavy macro machinery. To avoid useleless macro expansion
# errors, please write this:
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  target_compile_options(MyProject PRIVATE -fmacro-backtrace-limit=1)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(MyProject PRIVATE -ftrack-macro-expansion=0)
endif()
```

Another approach is downloading Metalang99 as a [Git submodule]; in this case, you can use CMake's [`add_subdirectory`]. Please, avoid directly copy-pasting `metalang99/include` to your project, because it will complicate updating to new versions of Metalang99 in the future.

[Git submodule]: https://git-scm.com/book/en/v2/Git-Tools-Submodules
[`add_subdirectory`]: https://cmake.org/cmake/help/latest/command/add_subdirectory.html

A few useful tips:

 - To reduce compilation times, you can try [precompiling headers] that rely on Metalang99 so that they will not be compiled each time they are included.
 - **PLEASE**, do not forget to specify [`-ftrack-macro-expansion=0`] (GCC), [`-fmacro-backtrace-limit=1`] (Clang), or something similar; otherwise, Metalang99 will throw your compiler to the moon.

[precompiling headers]: https://en.wikipedia.org/wiki/Precompiled_header
[`-ftrack-macro-expansion=0`]: https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html
[`-fmacro-backtrace-limit=1`]: https://clang.llvm.org/docs/ClangCommandLineReference.html#cmdoption-clang-fmacro-backtrace-limit

[Tutorial](https://hirrolot.gitbook.io/metalang99/) | [Examples](examples/) | [User documentation](https://metalang99.readthedocs.io/en/latest/)

Happy hacking!

## Highlights

 - **Macro recursion.** Recursive calls behave as expected. In particular, to implement recursion, [Boost/Preprocessor] just copy-pastes all recursive functions up to a certain limit and forces to either keep track of recursion depth or rely on a built-in deduction; Metalang99 is free from such drawbacks.

 - **Almost the same syntax.** Metalang99 does not look too alien in comparison with [Order PP] because the syntax differs insignificantly from usual preprocessor code.

 - **Partial application.** Instead of tracking auxiliary arguments here and there (as it is done in Boost/Preprocessor), partial application allows to capture an environment by applying constant values first. Besides that, partial application facilitates better reuse of metafunctions.

 - **Debugging and error reporting.** You can conveniently debug your macros with `ML99_abort` and report fatal errors with `ML99_fatal`. The interpreter will immediately finish its work and do the trick.

[Boost/Preprocessor]: http://boost.org/libs/preprocessor
[Order PP]: https://github.com/rofl0r/order-pp

## Philosophy and origins

My work on [Poica], a research programming language implemented upon [Boost/Preprocessor], has left me unsatisfied with the result. The fundamental limitations of Boost/Preprocessor have made the codebase simply unmaintainable; these include recursive macro calls (blocked by the preprocessor), which have made debugging a complete nightmare, the absence of partial application that has made context passing utterly awkward, and every single mistake that resulted in megabytes of compiler error messages.

Only then I have understood that instead of enriching the preprocessor with various ad-hoc mechanisms, we should really establish a clear paradigm in which to structure metaprograms. With these thoughts in mind, I started to implement Metalang99...

Long story short, it took half of a year of hard work to release v0.1.0 and almost a year to make it stable. As a real-world application of Metalang99, I created [Datatype99] exactly of the same form I wanted it to be: the implementation is highly declarative, the syntax is nifty, and the semantics is well-defined.

Finally, I want to say that Metalang99 is only about syntax transformations and not about CPU-bound tasks; the preprocessor is just too slow and limited for such kind of abuse.

[Poica]: https://github.com/Hirrolot/poica

## Guidelines

 - If possible, assert macro parameters for well-formedness using `ML99_assertIsTuple`, `ML99_assertIsNat`, etc. for better diagnostic messages.
 - Prefer the `##` token-pasting operator inside [Metalang99-compliant macros] instead of `ML99_cat` or its friends, because arguments will nevertheless be fully expanded.
 - Use [`ML99_todo` and its friends] to indicate unimplemented functionality.

[Metalang99-compliant macros]: https://metalang99.readthedocs.io/en/latest/#definitions
[`ML99_todo` and its friends]: https://metalang99.readthedocs.io/en/latest/util.html#c.ML99_todo

## Publications

 - [_What’s the Point of the C Preprocessor, Actually?_] by Hirrolot.
 - [_Macros on Steroids, Or: How Can Pure C Benefit From Metaprogramming_](https://hirrolot.github.io/posts/macros-on-steroids-or-how-can-pure-c-benefit-from-metaprogramming.html) by Hirrolot.
 - [_Extend Your Language, Don’t Alter It_](https://hirrolot.github.io/posts/extend-your-language-dont-alter-it.html) by Hirrolot.

[_What’s the Point of the C Preprocessor, Actually?_]: https://hirrolot.github.io/posts/whats-the-point-of-the-c-preprocessor-actually.html

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).

## Architecture

See [`ARCHITECTURE.md`](ARCHITECTURE.md).

## Idioms

See [`idioms.md`](idioms.md).

## Optimisation tips

See [`optimization_tips.md`](optimization_tips.md).

## FAQ

### Q: What about compile-time errors?

A: Metalang99 is a big step towards understandable compiler diagnostics. It has a built-in syntax checker that tests all incoming terms for validity:

[`playground.c`]
```c
ML99_EVAL(123)
ML99_EVAL(x, y, z)
ML99_EVAL(v(Billie) v(Jean))
```

[`/bin/sh`]
```
$ gcc playground.c -Imetalang99/include -ftrack-macro-expansion=0
playground.c:3:1: error: static assertion failed: "invalid term `123`"
    3 | ML99_EVAL(123)
      | ^~~~~~~~~
playground.c:4:1: error: static assertion failed: "invalid term `x`"
    4 | ML99_EVAL(x, y, z)
      | ^~~~~~~~~
playground.c:5:1: error: static assertion failed: "invalid term `(0v, Billie) (0v, Jean)`, did you miss a comma?"
    5 | ML99_EVAL(v(Billie) v(Jean))
      | ^~~~~~~~~
```

Metalang99 can even check for macro preconditions and report an error:

[`playground.c`]
```c
ML99_EVAL(ML99_listHead(ML99_nil()))
ML99_EVAL(ML99_unwrapLeft(ML99_right(v(123))))
ML99_EVAL(ML99_div(v(18), v(4)))
```

[`/bin/sh`]
```
$ gcc playground.c -Imetalang99/include -ftrack-macro-expansion=0
playground.c:3:1: error: static assertion failed: "ML99_listHead: expected a non-empty list"
    3 | ML99_EVAL(ML99_listHead(ML99_nil()))
      | ^~~~~~~~~
playground.c:4:1: error: static assertion failed: "ML99_unwrapLeft: expected ML99_left but found ML99_right"
    4 | ML99_EVAL(ML99_unwrapLeft(ML99_right(v(123))))
      | ^~~~~~~~~
playground.c:5:1: error: static assertion failed: "ML99_div: 18 is not divisible by 4"
    5 | ML99_EVAL(ML99_div(v(18), v(4)))
      | ^~~~~~~~~
```

However, if you do something awkward, compile-time errors can become quite obscured:

```c
// ML99_PRIV_REC_NEXT_ML99_PRIV_IF_0 blah(ML99_PRIV_SYNTAX_CHECKER_EMIT_ERROR, ML99_PRIV_TERM_MATCH) ((~, ~, ~) blah, ML99_PRIV_EVAL_)(ML99_PRIV_REC_STOP, (~), 0fspace, (, ), ((0end, ~), ~), ~, ~ blah)(0)()
ML99_EVAL((~, ~, ~) blah)
```

In either case, you can try to [iteratively debug your metaprogram](https://hirrolot.gitbook.io/metalang99/testing-debugging-and-error-reporting). From my experience, 95% of errors are comprehensible -- Metalang99 is built for humans, not for macro monsters.

### Q: What about debugging?

A: See the chapter [_Testing, debugging, and error reporting_](https://hirrolot.gitbook.io/metalang99/testing-debugging-and-error-reporting).

### Q: What about IDE support?

A: I use VS Code for development. It enables pop-up suggestments of macro-generated constructions but, of course, it does not support macro syntax highlighting.

### Q: Compilation times?

A: To run the benchmarks, execute `./scripts/bench.sh` from the root directory.

### Q: How does it work?

A:

 1. Because macro recursion is prohibited, there is an ad-hoc [recursion engine] which works by deferring macro expansions and passing continuations here and there.
 2. Upon it, the [continuation-passing style] [interpreter] reduces language expressions into final results.
 3. The standard library is nothing but a set of metafunctions implemented using the core metalanguage, i.e. they are to be evaluated by the interpreter.

[recursion engine]: include/metalang99/eval/rec.h
[interpreter]: include/metalang99/eval/eval.h
[continuation-passing style]: https://en.wikipedia.org/wiki/Continuation-passing_style

### Q: Why not third-party code generators?

A: See the blog post [_What’s the Point of the C Preprocessor, Actually?_]

### Q: Is it Turing-complete?

A: The C/C++ preprocessor is capable to iterate only [up to a certain limit](https://stackoverflow.com/questions/3136686/is-the-c99-preprocessor-turing-complete). For Metalang99, this limit is defined in terms of reductions steps: once a fixed amount of reduction steps has exhausted, your metaprogram will not be able to execute anymore.

### Q: Why macros if we have templates?

A: Metalang99 is primarily targeted at pure C, and C lacks templates. But anyway, you can find the argumentation for C++ at the website of [Boost/Preprocessor].

### Q: Where is an amalgamated header?

A: I am against amalgamated headers because of burden with updating. Instead, you can just add Metalang99 as a [Git submodule] and update it with `git submodule update --remote`.

### Q: Which standards are supported?

A: C99/C++11 and onwards.

[Git submodule]: https://git-scm.com/book/en/v2/Git-Tools-Submodules

### Q: Which compilers are tested?

A: Metalang99 is known to work on these compilers:

 - GCC
 - Clang
 - MSVC
 - TCC
