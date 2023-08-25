<h3 align="center">pg_yregress</h3>

<p align="center">
  regression testing tool for Postgres
  <br />
  <a href="https://docs.omnigres.org/pg_yregress/usage"><strong>Documentation</strong></a>
</p>

# Overview

Originally inspired by `pg_regress`, `pg_yregress` provides a TAP-compatible
test executor that allows for better test organization, easier instance
management and so on.

# Features

* [TAP](http://testanything.org/) reporting
* Grouped & structured tests
* Single-file test/expectation management
* Built-in JSON handling
* Binary type encoding testing
* Transaction management
* Deep instance configurability

# Examples

This test simply tests for the success of the query:

```yaml
- select 1
```

This tests the result:

```yaml
- query: select 1 as result
  results:
  - result: 1
```

This tests against JSON structures

```yaml
- query: select json_value from table
  results:
  - json_value:
      test: value
```

For more examples and use cases, check
out [documentation](https://docs.omnigres.org/pg_yregress/usage).

# Build & Install

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