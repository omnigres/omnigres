create type http_connection as
(
    http_protocol smallint,
    url           text
);

create function http_connections() returns setof http_connection
as
'MODULE_PATHNAME' language c;