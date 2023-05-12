create type http_request as
(
    method  omni_http.http_method,
    url     text,
    headers omni_http.http_headers,
    body    bytea
);

create function http_request(
    url text,
    method omni_http.http_method default 'GET',
    headers omni_http.http_headers default array []::omni_http.http_headers,
    body bytea default null
) returns http_request as
$$
select row (method, url, headers, body)
$$
    language sql
    immutable;


create type http_response as
(
    status  smallint,
    headers omni_http.http_headers,
    body    bytea
);

create function http_execute(request http_request) returns http_response
as
'MODULE_PATHNAME' language c;
