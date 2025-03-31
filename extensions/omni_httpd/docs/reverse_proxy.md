# Reverse Proxy

omni_httpd has a capability to respond to HTTP requests with other outcomes.
Of a particular interest is __`http_proxy`__ as it allows us to dynamically
proxy and re-process the incoming HTTP request to a backend:

```postgresql
select
    omni_httpd.http_proxy('http://127.0.0.1:9000' || request.path)
```

The above will simply redirect all incoming requests to `127.0.0.1` (port `9000`)
over plain HTTP with the request path being sent as-is.

This approach retrieving the target for proxying dynamically based
on data in the database, the incoming request itself or any other data
that can be retrieved in a query.

??? danger "Potential performance implications"

    This is a new feature and it hasn't been extensively benchmarked.
    Determining proxying information in runtime _may have_ some performance
    implications.

    In the future, we may provide a dedicated configuration for backend proxying
    that will allow for configuration-time resolution of the backends (for example,
    to fetch them from a table), if the performance implications will be too taxing
    in some use-cases.

## Additional Options

__`omni_httpd.http_proxy`__ takes the following optional parameters:

| Name | Description                                                 | Default |
|------|-------------------------------------------------------------|---------|
| `preserve_host` | Pass `Host` header from the incoming request to the backend | `true`    |