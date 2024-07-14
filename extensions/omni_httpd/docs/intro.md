# Intro

omni_httpd is an embedded HTTP server for Postgres. It allows one to write HTTP request handling in SQL.
This approach lends to less indirection that is common in traditional application architectures, where applications
are interacting with the database directly.

## Getting started.

By default, `omni_httpd`, once installed, will provide a default page on port `8080`.

You can change the handler by updating the `query` column in the `handlers` table. Currently, the idiomatic way
to write this query is to use `omni_httpd.cascading_query` aggregate function (though, of course, one can roll their
own SQL completely by hand). This function simplifies building priority-sorted request handling:

```postgresql
update omni_httpd.handlers
set
    query =
        (select
             omni_httpd.cascading_query(name, query order by priority desc nulls last)
         from
             (values
                  ('headers',
                   $$select omni_httpd.http_response(body => request.headers::text) from request where request.path = '/headers'$$,
                   1),
                  ('not_found',
                   $$select omni_httpd.http_response(status => 404, body => 'Not found') from request$$, 0))
                 as routes(name, query, priority));
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

    ```postgresql
    update omni_httpd.handlers
    set
        query =
            (select
                 omni_httpd.cascading_query(name, query order by priority asc nulls last) -- (1)
             from
                 (values
                      ('headers',
                       $$select omni_httpd.http_response(body => request.headers::text) from request where request.path = '/headers'$$,
                       1),
                      ('not_found',
                       $$select omni_httpd.http_response(status => 404, body => 'Not found') from request$$, 0))
                     as routes(name, query, priority));
    ```

    1. We changed the order from `desc` to `asc`

    Then `not_found` will always take the precedence:
    
    ```shell
    $ curl http://localhost:8080
    Not found
    $ curl http://localhost:8080/headers
    Not found
    ```

    !!! tip

        An interesting corollary to this approach is that if all of the handling sub-queries are of the same priority,
        then priority-ordering is not required and one can simply use `cascading_query` without `ORDER BY`.

??? question "What does `cascading_query` do?"

    The idea behind `cascading_query` is that it aggregates named queries in a `UNION` query where all given queries
    will become common table expressions (CTEs) and the `UNION` will be used to cascade over them, something like:

    ```postgresql
    with
        headers as (...),
        not_found as (...)
    select *
    from
        headers
    union all
    select *
    from
        not_found
    where
        not exists(select from headers)
    ```

All good. But looking back into the queries itself, they mention `request` which is nowhere to be found. Where does this
come from? This is actually a CTE that `omni_httpd` supplies in runtime that has the following `omni_httpd.http_request`
signature:

```postgresql
method omni_http.http_method,
path text,
query_string text,
body bytea,
headers omni_http.http_header[]
```

!!! tip

    If this signature seem a little incomplete (where's the source IP address, can the body by streamed, etc.?),
    that's because it is still work in progress. Please consider contributing if you feel up to it.

    Also, [omni_web](/omni_web/intro) provides complementary higher-level functionality.

In order to test your request handlers without having to run actual HTTP
requests against it, one can use `omni_httpd.http_request` function to compose
requests:

```postgresql
with
    request as (select (omni_httpd.http_request('/')).*)
select
    omni_httpd.http_response(request.path)
from
    request
```

|        Parameter | Type                   | Description                | Default     |
|-----------------:|------------------------|----------------------------|-------------|
|         **path** | text                   | Path                       | None        | 
|       **method** | omni_http.http_method  | HTTP method [^http-method] | `GET`       | 
| **query_string** | text                   | Query string               | `NULL`      | 
|      **headers** | omni_http.http_headers | An array of HTTP headers   | empty array |
|         **body** | bytea                  | Request body               | `NULL`      |

## Configuration

`omni_httpd` can be configured with the following PostgreSQL configuration variables:

* `omni_httpd.http_workers` to configure the number of http workers. It defaults to the number of cpus and adjusts if this is higher than what [max_worker_processes](https://www.postgresql.org/docs/current/runtime-config-resource.html#GUC-MAX-WORKER-PROCESSES) allows.
* `omni_httpd.temp_dir` to set the temporary directory for `omni_httpd` files like unix domain sockets, defaults
  to `/tmp`
