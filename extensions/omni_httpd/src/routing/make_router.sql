create or replace function make_router(
    router_name name,
    router_type name,
    route_regexp text default '.*',
    method text default 'get',
    host_regexp text default '.*'
) returns void as $$
declare
    type_columns text[];
    capture_count int;
    function_body text;
    named_captures text[];
begin
    -- Get the columns of the composite type
    select array_agg(attname::text)
    into type_columns
    from pg_attribute
    where attrelid = router_type::regclass
    and attnum > 0
    and not attisdropped;

    -- Generate function body
    function_body := format($func$
        create or replace function %I(omni_httpd.http_request) returns %I
        language plpgsql as
        $body$
        declare
            matches text[];
            match_value text;
        begin
            if $1.method != %L then
                return null;
            end if;

            if not $1.path ~ %L then
                return null;
            end if;

            matches := regexp_match($1.path, %L);
            if matches is null then
                return null;
            end if;

            return row($1, matches[1:])::$body$, 
                router_name, 
                router_type,
                method,
                route_regexp,
                -- Generate dynamic mapping from captures to columns
                array_to_string(array(
                    select format('%L := matches[%s]', 
                                  name, index)
                    from regex_named_groups(route_regexp)
                    where name = any (type_columns)
                ), ', ');

        end body$
    $func$, router_name, router_type, method, route_regexp);

    execute function_body;
end;
$$ language plpgsql;
