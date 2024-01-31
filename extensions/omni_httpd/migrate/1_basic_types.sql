create domain port integer check (value >= 0 and value <= 65535);

create type http_request as
(
    method       omni_http.http_method,
    path         text,
    query_string text,
    body         bytea,
    headers      http_headers
);

/*{% include "../src/http_request.sql" %}*/

create type http_response as
(
    body    bytea,
    status  smallint,
    headers http_headers
);

create type http_proxy as
(
    url           text,
    preserve_host boolean
);

create domain abort as omni_types.unit;

select omni_types.sum_type('http_outcome', 'http_response', 'abort', 'http_proxy');

/*{% include "../src/abort.sql" %}*/
/*{% include "../src/http_proxy.sql" %}*/

create function http_response(
    body anycompatible default null,
    status int default 200,
    headers http_headers default null
)
    returns http_outcome
as
'MODULE_PATHNAME',
'http_response'
    language c immutable;

create type http_protocol as enum ('http', 'https');