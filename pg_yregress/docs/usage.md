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

One can also check a `steps`-based test for acumulative sequence of notices
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