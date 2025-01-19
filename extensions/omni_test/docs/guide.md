# Testing Guide

`omni_test` allows you to run tests from within the database.

!!! tip "`omni_test` is a templated extension"

    `omni_test` is a templated extension. This means that by installing it, its default-instantiated
     into `omni_test` but can be instantiated into any other schema:

     ```postgresql 
     select omni_test.instantiate([schema => 'omni_test'])
     ```

In order to use it, you need to provision a template database with your test functions and everything they need:

```postgresql
create database myapp_test;
update pg_database set datistemplate = true where datname = 'myapp_test';
-- provision the content
```

!!! tip "Use `omni_schema.assemble_schema`"

    One of the easiest way to provision files into this new schema is to use
    `omni_schema.assemble_schema`:

    ```postgresql
    select * from 
      omni_schema.assemble_schema('dbname=myapp_test ..',
                                   omni_vfs.local_fs('/path/to/tests'))
    ```

## Writing tests

Tests are found by signature, they can be either **functions** or **procedures**. 

Functions must follow this signature:

```postgresql
create function my_test() returns omni_test.test -- ...
```

!!! question "What's `omni_test.test` type?"

    At this time, `omni_test.test` is am empty composite type and its value is ignored. It is simply
    used for finding tests. **This may change in the future**.


Procedures must follow this signature:

```postgresql
create procedure my_test(inout omni_test.test) -- ..
```

??? question "When to use procedures instead of functions?"

    Procedures are to be used if the test is to be **non-atomic**, that is, if it uses
    `commit` or `rollback`.

## Running tests

To run tests, simply pass the name of the database to the `run_tests` function:

```postgresql
select * from omni_test.run_tests('myapp_test')
```

Every test function and procedure is going to be executed in a fresh copy of the
`myapp_test` "template" database.

The results will conform to this structure:

|          **Name** | Type        | Description                           |
|------------------:|:------------|:--------------------------------------|
|          **name** | `text`      | The name of the test.                 |
|   **description** | `text`      | A detailed description of the test    |
|    **start_time** | `timestamp` | The start time of the test.           |
|      **end_time** | `timestamp` | The end time of the test.             |
| **error_message** | `text`      | An error message, if the test failed. |