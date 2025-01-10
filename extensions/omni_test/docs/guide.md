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

To run tests, simply pass the name of the database to the `run_tests` function:

```postgresql
select * from omni_test.run_tests('myapp_test')
```

The results will conform to this structure:

|          **Name** | Type        | Description                           |
|------------------:|:------------|:--------------------------------------|
|          **name** | `text`      | The name of the test.                 |
|   **description** | `text`      | A detailed description of the test    |
|    **start_time** | `timestamp` | The start time of the test.           |
|      **end_time** | `timestamp` | The end time of the test.             |
| **error_message** | `text`      | An error message, if the test failed. |