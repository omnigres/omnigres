create function postgrest_get_route(request omni_httpd.http_request) returns postgrest_get_route
    language plpgsql
as
$$
declare
    name  text;
    class regclass;
begin
    if request.method = 'GET' then
        name := split_part(request.path, '/', 2);
        class := to_regclass(name);
        if class is not null and class::text not like 'pg_%' then
            return row (request, class)::omni_rest.postgrest_get_route;
        end if;
    end if;
    return null;
end;
$$;