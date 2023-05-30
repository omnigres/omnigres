# Headers

HTTP requests come with headers, which can be retrieved using `omni_http.header_get` and `omni_http.header_get_all`
functions:

```sql
select omni_http.http_header_get(request.headers, 'host') as host;
select omni_http.http_header_get_all(request.headers, 'accept') as accept;
```

The header name these functions take is case-insensitive.