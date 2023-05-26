---
title: Usage of pg_yregress
---

# Usage of pg_yregress

Despite being inspired by `pg_regress`, `pg_yregress` is not in any way compatible with `pg_regress` as it has a different workflow and an execution model.

{% include-markdown "_install.md" heading-offset=1 %}

## Basic workflow

This tool uses YAML to describe tests. Let's start with
`test.yml`:

```yaml
tests:
- name: simple
  query: select 1 as value
```

The above specification will test the `select 1...` query be executing it and ensuring it was successful. The test will be executed against a
_managed instance_[^managed] of Postgres.

[^managed]: Postgres instance that is provisioned and deprovisioned by `pg_yregress` tool without any user involvement.

Running `pg_yregress` against this file will output the updated specification.

```shell
$ pg_yregress test.yaml
...
tests:
- name: simple
  query: select 1 as value
```

??? tip "The above test can be further simplified"

    There's a reduced syntax for checking whether query is successful
    without naming it. You can even drop the `query` key and simply write the query
    as a test:

    ```yaml
    - select 1 as value
    ```

Nothing very interesting. Now, let's amend this test to test the result of this query. For a moment, let's assume we don't know what results are to be returned.

```yaml
tests:
- name: simple
  query: select 1 as value
  results: [ ] # (1)
```

1. Here we specify an empty result set

Re-running the tool will output something different:

```shell
$ pg_yregress test.yaml
...
tests:
- name: simple
  query: select 1 as value
  results:
  - value: 1
```

As you can see, it shows what test specification __should__
contain in order to pass. You can also observe, that `pg_yregress`
exited with a __non-zero error code__.

For better visibility into changes, YAML-specific diff tools can be of use, such as [dyff](https://github.com/homeport/dyff). To make it easier to use these tools, `pg_yregress` takes an additional
_optional_ argument where it will output the updated specification instead of stdout.

Copying `results` to the original specification will make `pg_yregress`
return zero again (thus, signal that the specification is executed as expected.)

!!! tip

    Every `query` item is executed within an individual transaction that is rolled back
    at the end to ensure it does not interfere with other items.

## Handling JSON and JSONB

`pg_yregress` supports JSON types.

* If a supplied query parameter is a mapping or a sequence, it will be automatically converted to JSON strings
* If result value is of a JSON type, it will be converted to YAML value.

```yaml
- name: json and jsonb params
  query: select $1::json as json, $2::jsonb as jsonb
  params:
  - hello: 1
  - hello: 2
  results:
  - json:
      hello: 1
    jsonb:
      hello: 2

- name: json and jsonb results
  query: select json_build_object('hello', 1), jsonb_build_object('hello', 2)
  results:
  - json_build_object:
      hello: 1
    jsonb_build_object:
      hello: 2
```

## Testing for failures

You can simply test that a certain query will fail:

```yaml
tests:
- name: error
  query: selec 1 as value
  success: false
```

The above will succeed, since we have set `success` to
`false`.

But how we can test against specific error message? This can be done by adding `error` property:

```yaml
tests:
- name: error
  query: selec 1 as value
  error:
    severity: ERROR
    message: syntax error at or near "selec"
```

The above will pass as this is the error this test fails with.

Multiple forms of `error` report are supported:

### Error message

```yaml
error: syntax error at or near "selec"
```

When passed as a scalar value, error message will be compared with the provided one.

### Full error form

```yaml
error:
  severity: ERROR
  message: <error message>
  # Optional
  detail: <error details>
```

In this form, both severity and message can be specified.

## Multi-step tests

Some test inolve more than one query and we need to check for more than just the final result, so simply executing all statements and queries delimited by a semicolon wouldn't be great.

For this use-case, instead of using `query`, use `steps`:

```yaml
tests:
- name: Test
  steps:
  - query: create table tab as (select generate_series(1,3) as i)
  - query: select * from tab
    results:
    - i: 1
    - i: 2
    - i: 3
```

!!! tip

    The entire `steps` item is executed within an individual transaction and is rolled 
    back at the end to ensure it does not interfere with other items. Within `steps`,
    every item is not wrapped into a transaction and the results of each step are visible 
    in the next step.

## Committing Tests

By default, all tests are rolled back to ensure clean environment. However, in some cases, tests need to commit (for example, to test deferred constraints).

When this is necessary, the `commit` property of a test should be set to `false`:

```yaml
- query: insert into table values (...)
  commit: true
```

This can be also used for multi-step tests. If any of the steps is committed but the multi-step test itself isn't, it'll roll back the uncommitted steps.

## Notices

One can also check their tests for notices:

```yaml
tests:
- name: notices
  query: |
    do $$
      begin
        raise notice 'test 1';
        raise notice 'test 2';
      end;
    $$ language plpgsql
  notices:
  - test 1
  - test 2
```

One can also check a `steps`-based test the accumulated sequence of notices
(although testing individually in `query` steps is still possible):

```yaml
tests:
- name: multi-step notices (individual)
  steps:
  - query: |
      do $$
        begin
          raise notice 'test 1';
        end;
      $$ language plpgsql
    notices:
    - test 1
  - query: |
      do $$
        begin
          raise notice 'test 2';
        end;
      $$ language plpgsql
    notices:
    - test 2
```

## Binary format

Sometimes there's a need to test binary encoding of types[^send-recv]. `pg_yregress`
allows this to be done by manipulating the `binary` property of the `query` test.

| Value | Description |
|------|-------------|
| `true` | Both `params` and `results` are binary |
| `params` | `params` are binary |
| `results` | `results` are binary |

Binary encodings are done using hexadecimal notiation prefixed by `0x`.

This will return results as binary:

```yaml
tests:
- name: binary format
  query: select true as value
  binary: true
  results:
  - value: 0x01
```

And this will return results as characters but take parameters as binary:

```yaml
tests:
- name: binary format for params
  query: select $1::bool as value
  binary: params
  params:
  - 0x01
  results:
  - value: t
```

[^send-recv]: The encoding that is used by `SEND` and `RECEIVE` functions of the type.

## Skipping tests

If a test not meant to be executed, one can use `skip` directive to suppress its execution. Given a boolean scalar, if it is positive, the test will be skipped. If a negative boolean scalar will be given, it will not be skipped. If any other scalar will be given, it will be used as a reason for skipping the test.

```yaml
tests:
- name: skip this
  skip: true
- name: skip this for a reason
  skip: reason
```

Skipped tests don't need to have a valid instruction (`query` or `steps`).

## Configuring instances

Tests may have one more instances they run on. By default, `pg_yregress` will provision one. However, if you want to configure the instance or add more than one, you can use
`instances` configuration which is a mapping of names to the configuration dictionaries:

```yaml
instances:
  configured:
    # Can be configured with a mapping
    config:
      log_connections: yes
  configured_1:
    # Can be configured with a string using `postgresql.conf` format
    config: |
      log_connections = yes
  default:
    init:
    # Executes a sequence of queries
    - create extension my_extension
    # One instance may be specified as default 
    default: yes
  other:
    init:
    - alter system set config_param = '...'
    # Initialization may require restarting the instance
    - restart: true
```

Each test will run on a default instance, unless `instance` property is specified and the name of the instance is referenced.

## Configuring test suite

In certain cases, it may be useful to pass some configuration information to the test suite itself. While it is generally recommended to avoid this, sometimes it's right answer.

All test suites receive an implicit `env` mapping at the root that contains a mapping of all environment variables. Using YAML Path (YPath) notation, one can retrieve configuration specified through environment variables:

```yaml
- name: env
  query: select $1::text as user
  params:
  - */env/USER
  results:
  - user: */env/USER
```

## TAP output

`pg_yregress` can also provide a [TAP](https://testanything.org), Test Anything Protocol for human or machine consumption.

It is currently
_hidden_ in an output to file descriptor 1001 (if one is defined), and if you want to collect or print it, simply pipe that out to a program processing it (like `tapview'):

```shell
# Bash
$ pg_yregress test.yml 1001> >(tapview) 1>/dev/null 2>/dev/null
# Fish
$ pg_yregress test.yml 1>/dev/null 2>/dev/null 1001>|tapview
```

As the tool will evolve, there will likely be another way to activate this.