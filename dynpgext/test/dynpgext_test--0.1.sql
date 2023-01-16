CREATE FUNCTION loader_present()
    RETURNS bool
    AS 'MODULE_PATHNAME', 'loader_present'
    LANGUAGE C;