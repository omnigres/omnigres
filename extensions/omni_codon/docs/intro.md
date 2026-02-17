# Introduction

[Codon](https://exaloop.io) brings Python-like algorithmic convenience and familiarity wich a much higher performance.

In the examples below, we'll count prime numbers for a given limit, in Python and Codon.

```postgresql
create or replace function primes_py("limit" int8) returns int8
    language plpython3u as
$$
def is_prime(n):
    factors = 0
    for i in range(2, n):
        if n % i == 0:
            factors += 1
    return factors == 0
    
num = 0

for i in range(2, limit):
    if is_prime(i):
       num += 1
       
return num
$$
```

```postgresql
create or replace function primes_codon("limit" int8) returns int8
    language plcodon as
$$
def is_prime(n):
    factors = 0
    @par(schedule='dynamic')
    for i in range(2, n):
        if n % i == 0:
            factors += 1
    return factors == 0

@pg
def primes_codon(limit: int) -> int: 
    num = 0

    @par(schedule='dynamic')
    for i in range(2, limit):
        if is_prime(i):
           num += 1
           
    return num
$$
```

On some contemporary hardware at the time of writing (Apple M3 Max), when we call this function with a limit of 10,000,
it takes approximately 1630 ms (1.6 seconds) in Python. Codon version takes in the vicinity of 3 ms after the first
invocation (which takes about 175 ms, since it bear the overhead of compiling the function), which is a massive
improvement! Going further, Codon version for a limit of 100,000 took 300ms, and 1,000,000 took 23 seconds. We didn't
have enough patience to test these limits with Python.

## Usage

In order to implement a Codon function exported to Postgres, one must define a function matching its Postgres
definition, explicitly typed and annotated with `@pg`:

```postgresql
create or replace function is_prime("n" int8) returns bool
    language plcodon as
$$
@pg
def is_prime(n: int) -> bool:
    factors = 0
    for i in range(2, n):
        if n % i == 0:
            factors += 1
    return factors == 0
$$
```

## Supported Types

Currently, omni_codon's support for types is limited: `bool`, `int2`, `int4`, `int8`, `float4`, `float8` and `text`. New
types and arrays will be added in follow-up releases.

## Aggregates

Unsupported but will be added later.
