create type http_request as
(
    method      omni_http.http_method,
    url         text,
    headers     omni_http.http_headers,
    body bytea
);

create function http_request(
    url text,
    method omni_http.http_method default 'GET',
    headers omni_http.http_headers default array []::omni_http.http_headers,
    body bytea default null) returns http_request as
$$
select row (method, url, headers, body)
$$
    language sql
    immutable;


create type http_response as
(
    version smallint,
    status  smallint,
    headers omni_http.http_headers,
    body    bytea,
    error   text
);

create type http_execute_options as
(
    http2_ratio           smallint,
    http3_ratio           smallint,
    force_cleartext_http2 bool
);

create domain valid_http_execute_options as http_execute_options
    check ( (value).http2_ratio >= 0 and (value).http2_ratio <= 100 and
            (value).http3_ratio >= 0 and (value).http3_ratio <= 100 and
            ((value).http2_ratio + (value).http3_ratio) <= 100
        );

create function http_execute_options(http2_ratio integer default 0, http3_ratio integer default 0,
                                     force_cleartext_http2 bool default false) returns http_execute_options
as
$$
select row (http2_ratio, http3_ratio, force_cleartext_http2)::omni_httpc.valid_http_execute_options
$$ language sql immutable;

create function http_execute(variadic requests http_request[]) returns setof http_response
as
$$
select
    omni_httpc.http_execute_with_options(omni_httpc.http_execute_options(), variadic requests);
$$ language sql;

create function http_execute_with_options(options http_execute_options, variadic requests http_request[]) returns setof http_response
as
'MODULE_PATHNAME',
'http_execute' language c;

create type http_connection as
(
    http_protocol smallint,
    url           text
);

create function http_connections() returns setof http_connection
as
'MODULE_PATHNAME' language c;