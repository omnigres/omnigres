# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), 
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## unreleased

## 1.13.2 - 2022-05-15

### Fixed

 - Fix C++ compilation for `ML99_INTRODUCE_VAR_TO_STMT` and `ML99_INTRODUCE_NON_NULL_PTR_TO_STMT` ([issue #25](https://github.com/Hirrolot/metalang99/issues/25)).

## 1.13.1 - 2021-12-09

### Fixed

 - Specify `C` as the project language in `CMakeLists.txt`. Previously, CMake detected C++ and required a C++ compiler to compile the project.

## 1.13.0 - 2021-12-01

### Added

 - Add the root `CMakeLists.txt` to be able to use CMake with [`FetchContent`] or [`add_subdirectory`] ([PR #20](https://github.com/Hirrolot/metalang99/pull/20)).
 - `list.h`:
   - `ML99_listFilterMap` to filter a list with a maybe-returning function.

[`FetchContent`]: https://cmake.org/cmake/help/latest/module/FetchContent.html
[`add_subdirectory`]: https://cmake.org/cmake/help/latest/command/add_subdirectory.html

## 1.12.1 - 2021-11-23

### Deprecated

 - Deprecate `ML99_catEval` because there were no use cases over time.

## 1.12.0 - 2021-11-09

### Added

 - `choice.h`:
   - `ML99_choiceData`, `ML99_CHOICE_DATA` to extract a choice value data.
 - `gen.h`:
   - Add `ML99_fnPtr(Stmt)` to generate a function pointer.
 - New module `stmt.h`:
   - Take `ML99_INTRODUCE_VAR_TO_STMT`, `ML99_INTRODUCE_NON_NULL_PTR_TO_STMT`, `ML99_CHAIN_EXPR_STMT`, `ML99_CHAIN_EXPR_STMT_AFTER`, and `ML99_SUPPRESS_UNUSED_BEFORE_STMT` from `gen.h`.

### Changed

 - `choice.h`:
   - Define the representation of choice types as `(tag, ...)`.
 - `tuple.h`:
   - Emit a fatal error in `ML99_untuple` if an argument is not a tuple.
 - `gen.h`:
   - Move all statement chaining macros to `stmt.h` (see above).
   - Move `ML99_GEN_SYM` and `ML99_TRAILING_SEMICOLON` to `util.h`.

### Deprecated

 - `tuple.h`:
   - `ML99_untupleChecked` because it is the same as `ML99_untuple`.
   - `ML99_tupleEval`, `ML99_untupleEval` because there were no use cases over time.
 - `logical.h`:
   - Move all functions to `bool.h`.
 - `control.h`:
   - Move `ML99_OVERLOAD` to `variadics.h`.
   - Move `ML99_if`, `ML99_IF` to `bool.h`.
   - Move `ML99_repeat`, `ML99_times` to `gen.h`.

## 1.11.0 - 2021-10-02

### Added

 - New module `seq.h`:
   - `ML99_seqIsEmpty`, `ML99_SEQ_IS_EMPTY` to check for the empty sequence.
   - `ML99_seqGet`, `ML99_SEQ_GET` to get an `i`-indexed element.
   - `ML99_seqTail`, `ML99_SEQ_TAIL` to get a tail.
   - `ML99_seqForEach(I)` to iterate through each element.
 - `logical.h`:
   - `ML99_boolMatch(WithArgs)` to perform pattern matching on a boolean value.

### Changed

 - `list.h`, `variadics.h`:
   - Remove the requirement that `ML99_listFromTuples` and `ML99_variadicsForEach(I)` can accept at most 63 arguments.

## 1.10.0 - 2021-09-14

### Added

 - `util.h`:
   - `ML99_COMMA` that expands to a single comma.

### Deprecated

 - `util.h`:
   - `ML99_(L|R)PAREN` because they result in code that is difficult to reason about.

## 1.9.0 - 2021-08-27

### Added

 - `metalang99.h`:
   - `ML99_VERSION_COMPATIBLE` to check for a SemVer-compatible version.
   - `ML99_VERSION_EQ` to check for an exact version.

## 1.8.0 - 2021-08-26

### Added

 - `ident.h`:
   - `ML99_charLit`, `ML99_CHAR_LIT` to convert a Metalang99 character to a C character literal.

## 1.7.0 - 2021-08-13

### Changed

 - `assert.h`:
   - Generate `_Static_assert` from the assertion macros if compiling on C11.

## 1.6.0 - 2021-08-13

### Added

 - Tuple counterparts of variadics (`tuple.h`):
   - `ML99_tupleCount`, `ML99_TUPLE_COUNT`.
   - `ML99_tupleIsSingle`, `ML99_TUPLE_IS_SINGLE`.
   - `ML99_tupleForEach(I)`.

### Fixed

 - `util.h`:
   - `ML99_cat3` & `ML99_cat4` to desugar to themselves instead of `ML99_cat`.
 - `variadics.h`:
   - Make `variadics.h` work without including `nat.h` & `util.h`.

## 1.5.0 - 2021-08-11

### Added

 - `ML99_assignInitializerList(Stmt)` as `ML99_assign(Stmt)` counterparts for initialiser lists.

## 1.4.1 - 2021-08-05

### Fixed

 - Invalid C11 standard detection for `_Static_assert` ([issue #15](https://github.com/Hirrolot/metalang99/issues/15)).
 - Invalid C11 standard detection for `_Static_assert` on MSVC ([issue #16](https://github.com/Hirrolot/metalang99/issues/16)).

## 1.4.0 - 2021-08-02

### Added

- `ML99_ALLOW_POOR_DIAGNOSTICS`: if your compiler does not support decent diagnostic messages, Metalang99 will emit an error that can be suppressed by defining this macro.

### Fixed

 - Emit `_Static_assert` for diagnostics where possible:
   - C11.
   - Clang if `__has_extension(c_static_assert)`.
   - GCC if newer than [4.6](https://gcc.gnu.org/gcc-4.6/changes.html).

## 1.3.0 - 2021-07-24

### Added

 - `util.h`:
   - `ML99_todo(WithMsg)` and `ML99_unimplemented(WithMsg)` to indicate unimplemented functionality.

### Fixed

 - Handle the `(...) (...) ...` form in `ML99_isUntuple`:
   - All the dependent public functions inherit this ability too: `ML99_isTuple`, `ML99_untupleChecked`, and `ML99_listFromTuples`.
   - Now the interpreter is able to emit a syntax error for `v(123) v(456)`.

### Changed

 - Emit syntax errors and errors from `ML99_fatal` right to a console if compiling on GCC.

## 1.2.0 - 2021-06-06

### Added

 - `list.h`:
   - `ML99_listFromTuples` to transform comma-separated tuples into a list.
 - `util.h`:
   - `ML99_(L|R)PAREN` that expand to an opening/closing parenthesis.
 - `tuple.h`:
   - `ML99_untupleChecked` to emit a fatal error if a provided argument is not a tuple.
 - New module `ident.h`:
   - Migrate `ML99_detectIdent`, `ML99_identEq`, `ML99_DETECT_IDENT`, `ML99_IDENT_EQ`, `ML99_C_KEYWORD_DETECTOR`, `ML99_UNDERSCORE_DETECTOR` from `util.h`.
   - `ML99_(LOWER|UPPER)CASE_DETECTOR` to detect lower/uppercase characters.
   - `ML99_DIGIT_DETECTOR` to detect digits.
   - `ML99_char_eq`, `ML99_CHAR_EQ` to compare two characters.
   - `ML99_is(Lower|Upper)case`, `ML99_IS_(LOWER|UPPER)CASE` to check whether a letter is lower/uppercased.
   - `ML99_isDigit`, `ML99_IS_DIGIT` to check whether a character is digit.
   - `ML99_isChar`, `ML99_IS_CHAR` to check whether an identifier is a character.
   - `ML99_(LOWER|UPPER)CASE_CHARS` that expands to all comma-separated lower/uppercase characters.
   - `ML99_DIGITS` that expands to all comma-separated digits.

### Changed

 - `util.h`:
   - Automatically include `ident.h` for backwards compatibility.

### Fixed

 - Make Metalang99 work on TCC (see [datatype99/issues/10](https://github.com/Hirrolot/datatype99/issues/10)).

## 1.1.0 - 2021-04-24

### Added

 - `gen.h`:
   - Statement chaining macros:
     - `ML99_CHAIN_EXPR_STMT` to execute a statement before the next statement.
     - `ML99_CHAIN_EXPR_STMT_AFTER` to execute a statement afterwards.
     - `ML99_INTRODUCE_NON_NULL_PTR_TO_STMT` to introduce a non-null pointer to a statement.
   - Other:
     - `ML99_GEN_SYM` to generate unique identifiers.
     - `ML99_TRAILING_SEMICOLON` to force a trailing semicolon.
     - `ML99_semicoloned` that puts a semicolon after its argument.
     - `ML99_assign` to assign something to something.
     - `ML99_assignStmt` to generate an assignment statement.
     - `ML99_invoke` to invoke a macro/function.
     - `ML99_invokeStmt` to generate a macro/function invocation statement.
     - `ML99_prefixedBlock` to generate `prefix { code }`.
 - `util.h`:
   - Dealing with identifiers:
     - `ML99_detectIdent`.
     - `ML99_identEq`, `ML99_IDENT_EQ` to compare two identifiers.
     - `ML99_C_KEYWORD_DETECTOR` to detect the C11 keywords.
     - `ML99_UNDERSCORE_DETECTOR` to detect the underscore character (`_`).
   - Other:
     - `ML99_uncomma` to evaluate terms with the space-separator.
     - `ML99_reify` to reify a macro/function to a Metalang99-compliant metafunction.
     - `ML99_cat3`, `ML99_CAT3`, `ML99_CAT3_PRIMITIVE`.
     - `ML99_cat4`, `ML99_CAT4`, `ML99_CAT4_PRIMITIVE`.
 - `assert.h`:
   - `ML99_assert`, `ML99_assertEq`.
 - `variadics.h`:
   - `ML99_variadicsIsSingle`, `ML99_VARIADICS_IS_SINGLE`.
 - Built-in data type assertion macros:
   - `tuple.h`: `ML99_assertIsTuple`.
   - `nat.h`: `ML99_assertIsNat`.

### Fixed

 - `assert.h`:
   - Parenthesise expressions passed to `ML99_ASSERT`, `ML99_ASSERT_EQ`.

### Changed

 - `gen.h`:
   - `ML99_INTRODUCE_VAR_TO_STMT` can deal with several variables.

### Deprecated

 - `gen.h`:
   - `ML99_SUPPRESS_UNUSED_BEFORE_STMT` (use `ML99_CHAIN_EXPR_STMT((void)expr)` instead).

## 1.0.0 - 2021-03-27

### Added

 - `ML99_QUOTE`.

### Removed

 - `ML99_consume`, `ML99_CONSUME`.

### Changed

 - Move `ML99_TERMS` from `util.h` to `lang.h`.
 - Return a list of tuples from `ML99_listZip`, accept a list of tuples in `ML99_listUnzip`, return a tuple of lists from `ML99_listPartition`.
 - `ML99_listEval` => `ML99_LIST_EVAL`, `ML99_listEvalCommaSep` => `ML99_LIST_EVAL_COMMA_SEP`.
 - Accept ignored variadics in `ML99_nil`, `ML99_empty`, `ML99_true`, `ML99_false`, `ML99_nothing` (and their plain versions).

### Fixed

 - Emit the correct metafunction name in case of an error in `ML99_listGet`.
 - Remove a precondition that metafunctions passed to `ML99_listFoldl`, `ML99_listFolr`, `ML99_listFoldl1`, `ML99_listMap`, `ML99_listMapI`, `ML99_listFor`, `ML99_listMapInitLast`, and `ML99_listForInitLast` must evaluate to a single term.

## 0.5.0 - 2021-03-22

### Added

 - `ML99_SUPPRESS_UNUSED_BEFORE_STMT`.
 - `ML99_tupleGet`, `ML99_variadicsGet`, `ML99_TUPLE_GET`, `ML99_VARIADICS_GET`.
 - `ML99_tupleAppend`, `ML99_tuplePrepend`.
 - `ML99_indexedArgs`.
 - `ML99_appl4`.
 - `ML99_times`.
 - `ML99_TRUE`, `ML99_FALSE`.
 - `ML99_LEFT`, `ML99_RIGHT`, `ML99_IS_LEFT`, `ML99_IS_RIGHT`.
 - `ML99_JUST`, `ML99_NOTHING`, `ML99_IS_JUST`, `ML99_IS_NOTHING`.
 - `ML99_NAT_MAX`, `ML99_DIV_CHECKED`.
 - `gen.h`.

### Removed

 - `M_choiceEmpty(Plain)` (this allows a more optimal choice representation).
 - `M_semicolon` (this macro turned out to be [dangerous](https://github.com/Hirrolot/metalang99/commit/f17f06adf1a747a8897bbc90c598b2be21c945c8)).
 - `M_tupleHead`, `M_variadicsHead`.
 - `M_overload`.
 - `M_when(Plain)`, `M_whenLazy(Plain)`.
 - `M_putBefore`, `M_putAfter`, `M_putBetween`.
 - `M_leftUnderscored`, `M_rightUnderscored`.
 - `misc.h`, `eval.h`.

### Changed

 - Do not guarantee the exact number of available reduction steps, instead keep it "reasonable" for the practical needs.
 - Amalgamate `lang.h` with `eval.h`.
 - Employ the `SCREAMING_CASE` naming convention for plain macros.
 - All macros are prefixed with `ML99_`, unconditionally.
 - Accept a number as a first argument and a function as the second in `ML99_repeat`.
 - `M_get` => `ML99_listGet`.
 - `M_overloadPlain` => `ML99_OVERLOAD`.
 - `M_eval` => `ML99_EVAL`.
 - `M_callTrivial` => `ML99_callUneval`.
 - Move `ML99_repeat` from `misc.h` to `control.h`.
 - Move `ML99_indexed(Params, Fields, InitializerList, Args)` from `misc.h` to `gen.h`.
 - Move `ML99_braced`, `ML99_typedef`, `ML99_struct`, `ML99_anonStruct`, `ML99_union`, `ML99_anonUnion`, `ML99_enum`, `ML99_anonEnum` from `util.h` to `gen.h`.
 - `M_assertPlain` => `ML99_ASSERT_UNEVAL`, `M_assertEmptyPlain` => `ML99_ASSERT_EMPTY_UNEVAL`.
 - Rename "unsigned integers" to "natural numbers":
   - `uint.h` => `nat.h`.
   - `M_uintMatch(WithArgs)` => `ML99_natMatch(WithArgs)`.
   - `M_uintEq` => `ML99_natEq`.
   - `M_uintNeq` => `ML99_natNeq`.

### Fixed

 - Emit a compile-time error if [`/Zc:preprocessor`] (MSVC) was not specified.
 - Allow branches in `ML99_IF` expand to commas.

[`/Zc:preprocessor`]: https://docs.microsoft.com/en-us/cpp/build/reference/zc-preprocessor?view=msvc-160

## 0.4.2 - 2021-02-28

### Added

 - `METALANG99_MAJOR`, `METALANG99_MINOR`, `METALANG99_PATCH`.
 - `M_union`, `M_anonUnion`, `M_enum`, `M_anonEnum`.
 - `METALANG99_GCC_PRAGMA`, `METALANG99_CLANG_PRAGMA`.

### Fixed

 - Suppress Clang's `-Wshadow` for a variable produced by `M_INTRODUCE_VAR_TO_STMT`.

## 0.4.1 - 2021-02-28

### Added

 - `M_DETECT_IDENT`
 - `M_choicePlain`, `M_choiceEmptyPlain`, `M_consPlain`, `M_nilPlain`.
 - `M_listMapInPlace`, `M_listMapInPlaceI`.

### Changed

 - Increase the maximum arity from 16 to 255.
 - Specify the exact number of commas produced by `M_indexedInitializerList`.

### Fixed

 - Initialise variables produced by `M_semicolon` and `M_assertPlain` to suppress warnings.

## 0.4.0 - 2021-02-26

### Added

 - `tuple.h`: `M_tuple(Plain)`, `M_tupleEval`, `M_untuple(Plain)`, `M_untupleEval`, `M_isTuple(Plain)`, `M_isUntuple(Plain)`, `M_tupleHead(Plain)`, `M_tupleTail(Plain)`.

### Changed

 - Move the corresponding functions from `util.h` and `variadics.h` to `tuple.h`.

## 0.3.0 - 2021-02-26

### Added

 - `M_when`, `M_whenPlain`, `M_whenLazy`, `M_whenLazyPlain`.
 - `M_leftUnderscored`, `M_rightUnderscored`.
 - `M_INTRODUCE_VAR_TO_STMT`.
 - `M_terms`.
 - `M_tupleHead`, `M_tupleHeadPlain`, `M_tupleTail`, `M_tupleTailPlain`.
 - `M_indexedParams`, `M_indexedFields`, `M_indexedInitializerList`.
 - `M_typedef`, `M_struct`, `M_anonStruct`.
 - `M_choiceTag`, `M_choiceTagPlain`, `M_isNilPlain`, `M_isCons`, `M_isConsPlain`.

### Changed

 - Make `M_variadicsHead` accept a single argument too.
 - Now at most 63 variadic arguments are acceptable by `M_list`, `M_variadicsCount`, and `M_variadicsCountPlain`.
 - Terms now need to be separated by commas, e.g. instead of `v(1) M_call(F, v(2)) v(3)`, write `v(1), M_call(F, v(2)), v(3)` or `M_terms(v(1), M_call(F, v(2)), v(3)`.
 - The empty sequence is prohibited by `M_eval`, `M_call` and `M_abort`.
 - Use American style endings (because it is prevalent):
   - `M_(un)parenthesise(Eval)` => `M_(un)tuple(Eval)`.
   - `M_isParenthesised` => `M_isTuple`.
   - `M_isUnparenthesised` => `M_isUntuple`.
   - `M_parenthesisedVariadics(Head|Tail)` => `M_tuple(Head|Tail)`.
 - Shorten functions on unsigned integers:
   - `M_uintInc(Plain)` => `M_inc(Plain)`.
   - `M_uintDec(Plain)` => `M_dec(Plain)`.
   - `M_uintAdd(3)` => `M_add(3)`.
   - `M_uintSub(3)` => `M_sub(3)`.
   - `M_uintMul(3)` => `M_mul(3)`.
   - `M_uintDiv(3)` => `M_div(3)`.
   - `M_uintDivChecked` => `M_divChecked`.
   - `M_uintLesser(Eq)` => `M_lesser(Eq)`.
   - `M_uintGreater(Eq)` => `M_greater(Eq)`.
   - `M_uintMod` => `M_mod`.
   - `M_uintMin` => `M_min`.
   - `M_uintMax` => `M_max`.
 - `M_variadicsMap` => `M_variadicsForEach`, `M_variadicsMapI` => `M_variadicsForEachI`.

### Fixed

 - `aux.*` => `util.*` for compatibility with Windows.

### Removed

 - `M_variadicsMapCommaSep`, `M_variadicsMapICommaSep` (better use lists).
 - `M_const2`, `M_const3`.

## 0.2.0 - 2021-02-05

### Changed

 - The project name `Epilepsy` => `Metalang99` (more neutral).

### Fixed

 - Reporting about syntactic mismatches.

## 0.1.0 - 2021-02-04

### Added

 - This excellent project.
