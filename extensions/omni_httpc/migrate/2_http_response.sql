create type http_response as
(
    version smallint,
    status  smallint,
    headers omni_http.http_headers,
    body    bytea,
    error   text
);