# Intro

omni_httpd is an embedded HTTP server for Postgres. It allows one to write HTTP request handling in SQL.
This approach lends to less indirection that is common in traditional application architectures, where applications
are interacting with the database directly.

## Getting started.

By default, `omni_httpd`, once installed, will provide a default page on port `8080`.

You can start adding handlers to the [default router](routing.md#default-router). In order to do so,
we must first define a handler function:

```postgresql
create function my_handler(request omni_httpd.http_request)
  returns omni_httpd.http_outcome
  return omni_httpd.http_response(body => request.headers::text);
``` 

Now we can add it:

```postgresql
insert into omni_httpd.urlpattern_router (match, handler)
values (omni_httpd.urlpattern('/headers'), 'my_handler'::regproc);
```

A request to `/headers` will dump request's headers,. We can test it with `curl`:

```shell
$ curl http://localhost:8080/headers
{"(omnigres-connecting-ip,127.0.0.1)","(user-agent,curl/8.7.1)","(accept,*/*)"}
```

In order to test your request handlers without having to run actual HTTP
requests against it, one can use `omni_httpd.http_request` function to compose
requests:

```postgresql
select
    omni_httpd.http_response((omni_httpd.http_request('/')).path);
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

## Controls

To stop or start omni_httpd server, one can use `omni_httpd.stop()` and `omni_httpd.start()` procedures. They both
accept a boolean `immediate` parameter (defaults to `false`), which
will perform the operation immediately without waiting for the transaction to successfully commit.

!!! tip "When is it useful?"

    Beyond the most obvious scenario, in which you just need the server to be stopped,
    there's a case when the database it is running in requires to be free of users.
    For example, if you want to create another database with the primary database
    as its template (for example, for isolated testing).

    Bear in mind that currently, in order for the templated database to get its HTTP
    server running, you need to connect to it. This may change in the future.