CREATE TYPE http_method AS ENUM ('GET', 'HEAD', 'POST', 'PUT', 'DELETE', 'CONNECT', 'OPTIONS', 'TRACE', 'PATCH');

CREATE TYPE http_header AS (
    name text,
    value text,
    append bool
);

CREATE TYPE http_request AS (
    method http_method,
    path text,
    query_string text,
    body bytea,
    headers http_header[]
);

CREATE FUNCTION http_header(name text, value text, append bool DEFAULT false) RETURNS http_header AS $$
SELECT ROW(name, value, append) AS result;
$$
LANGUAGE SQL;

CREATE TYPE http_response AS (
    status smallint,
    headers http_header[],
    body bytea
);

CREATE FUNCTION http_response(
    status int DEFAULT 200,
    headers http_header[] DEFAULT array[]::http_header[],
    body anycompatible DEFAULT ''::bytea
)
    RETURNS http_response
    AS 'MODULE_PATHNAME', 'http_response'
    LANGUAGE C;

CREATE DOMAIN port integer CHECK (VALUE > 0 AND VALUE <= 65535);

CREATE TYPE listenaddress AS (
    addr inet,
    port port
);

CREATE TABLE listeners (
    id integer PRIMARY KEY GENERATED ALWAYS AS IDENTITY,
    listen listenaddress[] NOT NULL DEFAULT array[ROW('127.0.0.1', 80)::listenaddress],
    query text
);

CREATE FUNCTION reload_configuration_trigger() RETURNS trigger
    AS 'MODULE_PATHNAME', 'reload_configuration'
    LANGUAGE C;

CREATE FUNCTION reload_configuration() RETURNS bool
    AS 'MODULE_PATHNAME', 'reload_configuration'
    LANGUAGE C;

CREATE TRIGGER listeners_updated AFTER UPDATE OR DELETE OR INSERT ON listeners
EXECUTE FUNCTION reload_configuration_trigger();