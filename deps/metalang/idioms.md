# Idioms

_This document describes common idioms when using Metalang99, i.e., code patterns that have not been reified into abstractions yet._

## Detecting a keyword followed by parentheses

To detect something like `abracadabra(1, 2, 3)`, follow this simple pattern:

```c
#define DETECT_ABRACADABRA(x)               ML99_IS_TUPLE(ML99_CAT(DETECT_ABRACADABRA_, x))
#define DETECT_ABRACADABRA_abracadabra(...) ()

// 1
DETECT_ABRACADABRA(abracadabra(1, 2, 3))

// 0
DETECT_ABRACADABRA(blah)
```

## Extracting a value of a keyword followed by parentheses

To get `1, 2, 3` from `abracadabra(1, 2, 3)`:

```c
#define EXTRACT_ABRACADABRA(x)               ML99_CAT(EXTRACT_ABRACADABRA_, x)
#define EXTRACT_ABRACADABRA_abracadabra(...) __VA_ARGS__

// 1, 2, 3
EXTRACT_ABRACADABRA(abracadabra(1, 2, 3))
```

## Interspersing a comma

To intersperse a comma between one or more elements, put a comma before each element and pass them all to `ML99_variadicsTail`:

```c
#define ARRAY_SUBSCRIPTS(array, n)                                                                 \
    ML99_EVAL(ML99_variadicsTail(ML99_repeat(v(n), ML99_appl(v(GEN_SUBSCRIPT), v(array)))))
#define GEN_SUBSCRIPT_IMPL(array, i) v(, (array)[i])
#define GEN_SUBSCRIPT_ARITY          2

// (animals)[0], (animals)[1], (animals)[2]
ARRAY_SUBSCRIPTS(animals, 3)
```
