# Polyfills

!!! warning "Important installation information"

    In order to use polyfills provided by this extension you will need to set your `search_path` to list `omni_polyfill` **before** `pg_catalog`. This is important
    in order to ensure polyfills are attempted in the right order.

    ```postgresql
    set search_path to '$user', public, omni_polyfill, pg_catalog
    ```

## Polyfilled functions

* [trim_array](https://www.postgresql.org/docs/current/functions-array.html)
* [UUIDv7 family](uuidv7.md)