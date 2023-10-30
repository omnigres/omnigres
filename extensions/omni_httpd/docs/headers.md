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

If you set a lower `Content-Length`, the response body will be resized accordingly:

```sql
update
omni_httpd.handlers
set query = $$
  select *
  from omni_httpd.http_response(
    headers => array [omni_http.http_header('Content-Type', 'text/plain'), omni_http.http_header('Content-Length', '2')],
    body => 'Hi there'
  )
$$
where id = 1;
```

```bash
curl localhost:8080 -i

HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 2
Server: omni_httpd-0.1
Content-Type: text/plain

Hi
```

!!! warning "Overflowing Content-Length"

    If you set a Content-Length higher than the size of the response body, `omni_httpd` will
    emit a WARNING and defer to using the actual body size.
