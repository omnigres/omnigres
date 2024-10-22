CREATE OR REPLACE FUNCTION make_router(
    router_name name,
    router_type name,
    route_regexp text DEFAULT '.*',
    method text DEFAULT 'GET',
    host_regexp text DEFAULT '.*'
) RETURNS void AS $$
DECLARE
    type_columns text[];
    capture_count int;
    function_body text;
    type_definition text;
BEGIN
    -- Get the columns of the composite type
    SELECT array_agg(attname::text)
    INTO type_columns
    FROM pg_attribute
    WHERE attrelid = router_type::regclass
    AND attnum > 0
    AND NOT attisdropped;

    -- Count capturing groups in route_regexp
    SELECT regexp_matches(route_regexp, '\((?!\?:)', 'g')
    INTO capture_count;

    -- Generate function body
    function_body := format($func$
CREATE OR REPLACE FUNCTION %I(omni_httpd.http_request) RETURNS %I
    LANGUAGE plpgsql AS
$body$
DECLARE
    matches text[];
BEGIN
    IF $1.method != %L THEN
        RETURN NULL;
    END IF;

    IF NOT $1.path ~ %L THEN
        RETURN NULL;
    END IF;

    IF NOT $1.host ~ %L THEN
        RETURN NULL;
    END IF;

    matches := regexp_match($1.path, %L);
    IF matches IS NULL THEN
        RETURN NULL;
    END IF;

    RETURN ROW($1, matches[1:])::$body$, 
        router_name, 
        router_type,
        method,
        route_regexp,
        host_regexp,
        route_regexp);

    EXECUTE function_body || router_type || ';
END body$
$func$';

    EXECUTE function_body;
END;
$$ LANGUAGE plpgsql;