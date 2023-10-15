<h3 align="center">pg_yregress</h3>

<p align="center">
  regression testing tool for Postgres
  <br />
  <a href="https://docs.omnigres.org/pg_yregress/usage"><strong>Documentation</strong></a>
</p>

# Overview

Originally inspired by `pg_regress`, `pg_yregress` provides
a [TAP](https://testanything.org)-compatible test executor that allows for
better test organization, easier instance management, native JSON handling and
so on.

## What's the difference?

`pg_regress` drives a `psql` session and your tests are essentially an stdout
capture.

`pg_yregress` organizes tests and expectations in a YAML document, making tests
easier to maintain and use.

# Features

* [TAP](https://testanything.org/) reporting
* Grouped & structured tests
* Single-file test/expectation management
* Built-in JSON handling
* Binary type encoding testing
* Transaction management
* Deep instance configurability

# Use cases

* Testing Postgres extensions
* Verifying complex SQL queries or behaviors
* Stored function testing
* Data health checks

# Examples

Tests are written in YAML files (for clarity) and can be executed using
`pg_yregress <file.yml>`.

This test simply tests for the success of the query:

```yaml
tests:
- select 1
```

This tests the result:

```yaml
tests:
- query: select 1 as result
  results:
  - result: 1
```

This tests against JSON structures

```yaml
tests:
- query: select json_value from table
  results:
  - json_value:
      test: value
```

For more examples and use cases, check
out [documentation](https://docs.omnigres.org/pg_yregress/usage).

# Install

## macOS (Homebrew)

```shell
brew install omnigres/omnigres/pg_yregress
```

## From sources

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
# Install
cmake --install build
```

This build will be using user- or system-installed Postgres.

# Running

```shell
pg_yregress test.yml
```

It will print TAP output for all the tests. To simplify the output, TAP printers
like [tapview](https://gitlab.com/esr/tapview/-/blob/master/tapview) can be
used:

```shell
$ pg_yregress test.yml | tapview
...........................................................................ss.sss..xx.uuu...
92 tests, 0 failures, 5 TODOs, 5 SKIPs.
```

# Community

* [Discord server](https://discord.gg/A2KxpjfQus): come and chat with out
  pg_yregress users.

# References

* [Inaugural blog post](https://yrashk.com/blog/2023/04/23/structured-postgres-regression-tests/)