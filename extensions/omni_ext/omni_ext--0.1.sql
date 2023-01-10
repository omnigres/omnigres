CREATE FUNCTION load(name cstring, version cstring DEFAULT NULL)
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'load'
    LANGUAGE C;

CREATE FUNCTION unload(name cstring, version cstring)
    RETURNS bool
    AS 'MODULE_PATHNAME', 'unload'
    LANGUAGE C;