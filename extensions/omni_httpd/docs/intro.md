# Intro

omni_httpd is an embedded HTTP server for Postgres. It allows one to write HTTP request handling in SQL.
This approach lends to less indirection that is common in traditional application architectures, where applications
are interacting with the database directly.

## Getting started.

By default, `omni_httpd`, once installed, will provide a default page on port `8080`.

You can change the handler by updating the `query` column in the `handlers` table. Currently, the idiomatic way
to write this query is to use `omni_httpd.cascading_query` aggregate function (though, of course, one can roll their
own SQL completely by hand). This function simplifies building priority-sorted request handling:

```sql
UPDATE omni_httpd.handlers SET query = 
(SELECT omni_httpd.cascading_query(name, query) FROM (VALUES
      ('headers',
      $$SELECT omni_httpd.http_response(body => request.headers::text) FROM request WHERE request.path = '/headers'$$),
      ('not_found',
      $$SELECT omni_httpd.http_response(status => 404, body => 'Not found') FROM request$$))
     AS routes(name,query));
```

??? tip "What if the query is invalid?"

    omni_httpd enforces validity of the query using a constraint trigger at the transaction boundary when updating or
    inserting a handler. This means that once the transaction is being committed, the query is validated and if it,
    say, refers to an unknown relation, column or is invalid for other reasons, it will be rejected and the transaction
    will not succeed.

    Please note, however, that at this moment, this enforcement will not help avoiding runtime errors if you render
    your query invalid *afterwards* (for example, by dropping relations it references), this will lead to runtime errors, ultimately
    leading to HTTP 500 responses.

The query called `headers` will dump request's headers, `not_found` will return HTTP 404. We can test it with `curl`:

```shell
$ curl http://localhost:8080
Not found
$ curl http://localhost:8080/headers
{"(user-agent,curl/7.86.0,t)","(accept,*/*,t)"}
```

The above method of defining the handler can work well when the queries that it is composed of are either stored in a
database
or can be retrieved during deployment (say, from a Git repository or any other source.)

??? question "What did you mean by "priority-sorted" request handling?"

    If you look at the order of handlers we added (`headers` followed by
    `not_found`), it means that `cascading_query`-built query will first try to get results from `headers` and if none available, will
    attempt `not_found`. Suppose we changed the order:

    ```sql
    UPDATE omni_httpd.handlers SET query = 
    (SELECT omni_httpd.cascading_query(name, query) FROM (VALUES 
          ('not_found',
          $$SELECT omni_httpd.http_response(status => 404, body => 'Not found') FROM request$$),
          ('headers',
          $$SELECT omni_httpd.http_response(body => request.headers::text) FROM request WHERE request.path = '/headers'$$))
         AS routes(name,query));
    ```
    
    Then `not_found` will always take the precedence:
    
    ```shell
    $ curl http://localhost:8080
    Not found
    $ curl http://localhost:8080/headers
    Not found
    ```

??? question "What does `cascading_query` do?"

    The idea behind `cascading_query` is that it aggregates named queries in a `UNION` query where all given queries
    will become common table expressions (CTEs) and the `UNION` will be used to cascade over them, something like:

    ```sql
    WITH headers AS (...),
         not_found AS (...)
    SELECT * FROM headers
    UNION ALL
    SELECT * FROM not_found WHERE NOT EXISTS (SELECT FROM headers)
    ```

All good. But looking back into the queries itself, they mention `request` which is nowhere to be found. Where does this
come from? This is actually a CTE that `omni_httpd` supplies in runtime that has the following `omni_httpd.http_request`
signature:

```sql
method omni_httpd.http_method,
path text,
query_string text,
body bytea,
headers omni_httpd.http_header[]
```

!!! tip

    If this signature seem a little incomplete (where's the source IP address, can the body by streamed, etc.?),
    that's because it is still work in progress. Please consider contributing if you feel up to it.