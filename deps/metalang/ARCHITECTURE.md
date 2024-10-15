# Architecture

_This document describes the high-level architecture of Metalang99._

## Interpreter

The interpreter interprets the core metalanguage described in the [specification].

[specification]: https://github.com/Hirrolot/metalang99/blob/master/spec/spec.pdf

### `eval/eval.h`

`eval/eval.h` exposes a single macro `ML99_PRIV_EVAL` which evaluates a given metaprogram. It is implemented as a machine in [continuation-passing style] which is described in the specification too.

[continuation-passing style]: https://en.wikipedia.org/wiki/Continuation-passing_style

### `eval/rec.h`

`eval/rec.h` contains a macro recursion engine upon which everything executes.

## Standard library

The Metalang99 standard library is a set of functions implemented using the core metalanguage. They are located inside corresponding files listed at the [documentation]'s front page.

[documentation]: https://metalang99.readthedocs.io/en/latest/
