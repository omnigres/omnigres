# Query Strings

## Parsing query strings

`parse_query_string` takes a query string:

```sql
SELECT omni_web.parse_query_string('key=value')
```

And returns an array of keys and values:

```psql
 parse_query_string 
--------------------
 {key,value}
(1 row)
```

!!! tip

    It can be used together with Postgres built-in
    function [`jsonb_object`](https://www.postgresql.org/docs/current/functions-json.html)
    if the keys are expected to be unique:

    ```sql
    SELECT jsonb_object(omni_web.parse_query_string('key=value'))->'key';
    ```

    ```psql
     ?column?
    ----------
     "value"
    (1 row)
    ```
