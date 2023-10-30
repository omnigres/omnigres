# Headers

HTTP requests come with headers, which can be retrieved using `omni_http.header_get` and `omni_http.header_get_all`
functions:

```sql
select omni_http.http_header_get(request.headers, 'host') as host;
select omni_http.http_header_get_all(request.headers, 'accept') as accept;
```

The header name these functions take is case-insensitive.

## Content-Length

`omni_httpd` automatically sets the `Content-Length` header for any non-null response body.

For instance, having:

```sql
create extension omni_httpd cascade;

update
omni_httpd.handlers
set query = $$
  select *
  from omni_httpd.http_response(
    headers => array [omni_http.http_header('Content-Type', 'text/plain')],
    body => 'Hi there'
  )
$$
where id = 1;
```

Will produce the response:

```bash
curl localhost:8080 -i

HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 8
Server: omni_httpd-0.1
Content-Type: text/plain

Hi there
```

!!! warning "Overriding Content-Length"

    `omni_httpd` allows overriding the `Content-Length`. This is useful for integrating with other HTTP handlers (e.g. Flask) that set the `Content-Length`.
    To ensure correctness, overriding works in the following way:

    - If the `Content-Length` is set lower than the actual body size. `omni_httpd` will use the new `Content-Length` and downsize the response body.
    - If the `Content-Length` is set higher than the actual body size. `omni_httpd` will keep its `Content-Length`, emit a WARNING and use the actual body size.
