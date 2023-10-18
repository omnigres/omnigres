# HTTP client

omni_httpc provides an efficient HTTP/1, HTTP/2 and HTTP/3 (experimental) client functionality.

## Standard mode

The basic mode for `omni_httpc` use is through the use of functions provided by the extension.

### Preparing a request

In order to prepare a request, one can use `omni_httpc.http_request()` function, with the following
parameters:

|   Parameter | Type                   | Description                | Default                 |
|------------:|------------------------|----------------------------|-------------------------|
|     **url** | text                   | URL                        | None [^null-is-illegal] | 
|  **method** | omni_http.http_method  | HTTP method [^http-method] | `GET`                   | 
| **headers** | omni_http.http_headers | An array of HTTP headers   | None                    |
|    **body** | bytea                  | Request body               | `NULL`                  |
 

[^null-is-illegal]: NULL is illegal
[^http-method]: `GET`, `HEAD`, `POST`, `PUT`, `DELETE`, `CONNECT`, `OPTIONS`, `TRACE`, `PATCH`

The function returns a prepared request. No request is executed at this point.

!!! tip "Null values in headers"

    If header name is `null`, it won't create any header. If header value is 
    `null`, it'll be serialized as an empty string.

### Executing requests

Requests can be executed using `omni_httpc.http_execute` functions which takes a variadic array
of requests (which means you can execute more than one request at a time):

```postgresql
select 
    version >> 8 as http_version, status, headers,
    convert_from(body, 'utf-8') 
from 
    omni_httpc.http_execute(
        omni_httpc.http_request('https://example.com'), 
        omni_httpc.http_request('https://example.org'))
```

Produces

```
-[ RECORD 1 ]+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
http_version | 1
status       | 200
headers      | {"(age,500113)","(cache-control,max-age=604800)","(content-type,\"text/html; charset=UTF-8\")","(date,\"Mon, 29 May 2023 23:45:47 GMT\")","(etag,\"\"\"3147526947+ident\"\"\")","(expires,\"Mon, 05 Jun 2023 23:45:47 GMT\")","(last-modified,\"Thu, 17 Oct 2019 07:18:26 GMT\")","(server,\"ECS (sec/96ED)\")","(vary,Accept-Encoding)","(x-cache,HIT)","(content-length,1256)"}
convert_from | <!doctype html>                                                                                                                                                                                                                                                                                                                                                                  +
             | <html>                                                                                                                                                                                                                                                                                                                                                                           +
             | <head>                                                                                                                                                                                                                                                                                                                                                                           +
             |     <title>Example Domain</title>                                                                                                                                                                                                                                                                                                                                                +
             |                                                                                                                                                                                                                                                                                                                                                                                  +
             |     <meta charset="utf-8" />                                                                                                                                                                                                                                                                                                                                                     +
             |     <meta http-equiv="Content-type" content="text/html; charset=utf-8" />                                                                                                                                                                                                                                                                                                        +
             |     <meta name="viewport" content="width=device-width, initial-scale=1" />                                                                                                                                                                                                                                                                                                       +
             |     <style type="text/css">                                                                                                                                                                                                                                                                                                                                                      +
             |     body {                                                                                                                                                                                                                                                                                                                                                                       +
             |         background-color: #f0f0f2;                                                                                                                                                                                                                                                                                                                                               +
             |         margin: 0;                                                                                                                                                                                                                                                                                                                                                               +
             |         padding: 0;                                                                                                                                                                                                                                                                                                                                                              +
             |         font-family: -apple-system, system-ui, BlinkMacSystemFont, "Segoe UI", "Open Sans", "Helvetica Neue", Helvetica, Arial, sans-serif;                                                                                                                                                                                                                                      +
             |                                                                                                                                                                                                                                                                                                                                                                                  +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     div {                                                                                                                                                                                                                                                                                                                                                                        +
             |         width: 600px;                                                                                                                                                                                                                                                                                                                                                            +
             |         margin: 5em auto;                                                                                                                                                                                                                                                                                                                                                        +
             |         padding: 2em;                                                                                                                                                                                                                                                                                                                                                            +
             |         background-color: #fdfdff;                                                                                                                                                                                                                                                                                                                                               +
             |         border-radius: 0.5em;                                                                                                                                                                                                                                                                                                                                                    +
             |         box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);                                                                                                                                                                                                                                                                                                                            +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     a:link, a:visited {                                                                                                                                                                                                                                                                                                                                                          +
             |         color: #38488f;                                                                                                                                                                                                                                                                                                                                                          +
             |         text-decoration: none;                                                                                                                                                                                                                                                                                                                                                   +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     @media (max-width: 700px) {                                                                                                                                                                                                                                                                                                                                                  +
             |         div {                                                                                                                                                                                                                                                                                                                                                                    +
             |             margin: 0 auto;                                                                                                                                                                                                                                                                                                                                                      +
             |             width: auto;                                                                                                                                                                                                                                                                                                                                                         +
             |         }                                                                                                                                                                                                                                                                                                                                                                        +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     </style>                                                                                                                                                                                                                                                                                                                                                                     +
             | </head>                                                                                                                                                                                                                                                                                                                                                                          +
             |                                                                                                                                                                                                                                                                                                                                                                                  +
             | <body>                                                                                                                                                                                                                                                                                                                                                                           +
             | <div>                                                                                                                                                                                                                                                                                                                                                                            +
             |     <h1>Example Domain</h1>                                                                                                                                                                                                                                                                                                                                                      +
             |     <p>This domain is for use in illustrative examples in documents. You may use this                                                                                                                                                                                                                                                                                            +
             |     domain in literature without prior coordination or asking for permission.</p>                                                                                                                                                                                                                                                                                                +
             |     <p><a href="https://www.iana.org/domains/example">More information...</a></p>                                                                                                                                                                                                                                                                                                +
             | </div>                                                                                                                                                                                                                                                                                                                                                                           +
             | </body>                                                                                                                                                                                                                                                                                                                                                                          +
             | </html>                                                                                                                                                                                                                                                                                                                                                                          +
             | 
-[ RECORD 2 ]+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
http_version | 1
status       | 200
headers      | {"(age,21859)","(cache-control,max-age=604800)","(content-type,\"text/html; charset=UTF-8\")","(date,\"Mon, 29 May 2023 23:45:47 GMT\")","(etag,\"\"\"3147526947+ident\"\"\")","(expires,\"Mon, 05 Jun 2023 23:45:47 GMT\")","(last-modified,\"Thu, 17 Oct 2019 07:18:26 GMT\")","(server,\"ECS (sec/96EE)\")","(vary,Accept-Encoding)","(x-cache,HIT)","(content-length,1256)"}
convert_from | <!doctype html>                                                                                                                                                                                                                                                                                                                                                                  +
             | <html>                                                                                                                                                                                                                                                                                                                                                                           +
             | <head>                                                                                                                                                                                                                                                                                                                                                                           +
             |     <title>Example Domain</title>                                                                                                                                                                                                                                                                                                                                                +
             |                                                                                                                                                                                                                                                                                                                                                                                  +
             |     <meta charset="utf-8" />                                                                                                                                                                                                                                                                                                                                                     +
             |     <meta http-equiv="Content-type" content="text/html; charset=utf-8" />                                                                                                                                                                                                                                                                                                        +
             |     <meta name="viewport" content="width=device-width, initial-scale=1" />                                                                                                                                                                                                                                                                                                       +
             |     <style type="text/css">                                                                                                                                                                                                                                                                                                                                                      +
             |     body {                                                                                                                                                                                                                                                                                                                                                                       +
             |         background-color: #f0f0f2;                                                                                                                                                                                                                                                                                                                                               +
             |         margin: 0;                                                                                                                                                                                                                                                                                                                                                               +
             |         padding: 0;                                                                                                                                                                                                                                                                                                                                                              +
             |         font-family: -apple-system, system-ui, BlinkMacSystemFont, "Segoe UI", "Open Sans", "Helvetica Neue", Helvetica, Arial, sans-serif;                                                                                                                                                                                                                                      +
             |                                                                                                                                                                                                                                                                                                                                                                                  +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     div {                                                                                                                                                                                                                                                                                                                                                                        +
             |         width: 600px;                                                                                                                                                                                                                                                                                                                                                            +
             |         margin: 5em auto;                                                                                                                                                                                                                                                                                                                                                        +
             |         padding: 2em;                                                                                                                                                                                                                                                                                                                                                            +
             |         background-color: #fdfdff;                                                                                                                                                                                                                                                                                                                                               +
             |         border-radius: 0.5em;                                                                                                                                                                                                                                                                                                                                                    +
             |         box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);                                                                                                                                                                                                                                                                                                                            +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     a:link, a:visited {                                                                                                                                                                                                                                                                                                                                                          +
             |         color: #38488f;                                                                                                                                                                                                                                                                                                                                                          +
             |         text-decoration: none;                                                                                                                                                                                                                                                                                                                                                   +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     @media (max-width: 700px) {                                                                                                                                                                                                                                                                                                                                                  +
             |         div {                                                                                                                                                                                                                                                                                                                                                                    +
             |             margin: 0 auto;                                                                                                                                                                                                                                                                                                                                                      +
             |             width: auto;                                                                                                                                                                                                                                                                                                                                                         +
             |         }                                                                                                                                                                                                                                                                                                                                                                        +
             |     }                                                                                                                                                                                                                                                                                                                                                                            +
             |     </style>                                                                                                                                                                                                                                                                                                                                                                     +
             | </head>                                                                                                                                                                                                                                                                                                                                                                          +
             |                                                                                                                                                                                                                                                                                                                                                                                  +
             | <body>                                                                                                                                                                                                                                                                                                                                                                           +
             | <div>                                                                                                                                                                                                                                                                                                                                                                            +
             |     <h1>Example Domain</h1>                                                                                                                                                                                                                                                                                                                                                      +
             |     <p>This domain is for use in illustrative examples in documents. You may use this                                                                                                                                                                                                                                                                                            +
             |     domain in literature without prior coordination or asking for permission.</p>                                                                                                                                                                                                                                                                                                +
             |     <p><a href="https://www.iana.org/domains/example">More information...</a></p>                                                                                                                                                                                                                                                                                                +
             | </div>                                                                                                                                                                                                                                                                                                                                                                           +
             | </body>                                                                                                                                                                                                                                                                                                                                                                          +
             | </html>                                                                                                                                                                                                                                                                                                                                                                          +
             | 
```

#### Response columns

|      Column | Type                   | Description                                     |
|------------:|------------------------|-------------------------------------------------|
| **version** | smallint               | http_major << 8 + http_minor [^http-version]    |
|  **status** | smallint               | HTTP response status (200, 404, etc.)           |
| **headers** | omni_http.http_headers | Response headers                                |
|    **body** | bytea                  | Response body                                   |
|   **error** | text                   | If not `NULL`, an error occurred during request |


[^http-version]: This will likely be changed in the upcoming release


### Configuring request execution

It is also possible to configure certain parameters of request execution. 

In this example, we're making `omni_httpc` switch to HTTP/2:

```postgresql
select 
    version >> 8 as http_version, status, headers 
from
    omni_httpc.http_execute_with_options(omni_httpc.http_execute_options(http2_ratio => 100), 
        omni_httpc.http_request('https://example.com'),
        omni_httpc.http_request('https://example.org'))
```

You can now see that `http_version` is set to `2`:

```
-[ RECORD 1 ]+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
http_version | 2
status       | 200
headers      | {"(age,592467)","(cache-control,max-age=604800)","(content-type,\"text/html; charset=UTF-8\")","(date,\"Mon, 29 May 2023 23:49:05 GMT\")","(etag,\"\"\"3147526947+ident\"\"\")","(expires,\"Mon, 05 Jun 2023 23:49:05 GMT\")","(last-modified,\"Thu, 17 Oct 2019 07:18:26 GMT\")","(server,\"ECS (sec/976A)\")","(vary,Accept-Encoding)","(x-cache,HIT)","(content-length,1256)"}
-[ RECORD 2 ]+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
http_version | 2
status       | 200
headers      | {"(age,500311)","(cache-control,max-age=604800)","(content-type,\"text/html; charset=UTF-8\")","(date,\"Mon, 29 May 2023 23:49:05 GMT\")","(etag,\"\"\"3147526947+ident\"\"\")","(expires,\"Mon, 05 Jun 2023 23:49:05 GMT\")","(last-modified,\"Thu, 17 Oct 2019 07:18:26 GMT\")","(server,\"ECS (sec/96ED)\")","(vary,Accept-Encoding)","(x-cache,HIT)","(content-length,1256)"}
```

#### Options

|                    Option | Type     | Description                                                                     | Default |
|--------------------------:|----------|---------------------------------------------------------------------------------|---------|
|           **http2_ratio** | smallint | Percentage of requests to be attempted with HTTP/2 `(0..100)` [^ratio]          | `0`     |
|           **http3_ratio** | smallint | Percentage of requests to be attempted with HTTP/3 `(0..100)` [^ratio] | `0`     |
| **force_cleartext_http2** | bool     | Allow HTTP/2 to be used without SSL                                             | `false` |

!!! tip "More options will be added in the near future"

[^ratio]: The sum of `http2_ratio` and `http3_ratio` must not exceed `100`

### Connection Pool

In every Postgres process, `omni_httpc` maintains a connection pool shared across function calls. You can inspect it by calling
`omni_httpc.http_connections`:

```postgresql
select * from omni_httpc.http_connections
```

Sample output:

```
http_protocol |     url     
---------------+-------------
             2 | example.com
             2 | example.org
(2 rows)
```

## Background mode

!!! danger "Not yet supported"

    There's a plan to add a capability for processing HTTP requests in background, executing
    SQL queries upon request completion.

    However, this work has not been done yet. You're welcome to [contribute](https://github.com/omnigres/omnigres)
