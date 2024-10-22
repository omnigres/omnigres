create function fallback_route(omni_httpd.http_request) returns fallback_route
    language plpgsql as
$$
begin
    return row ($1)::omni_rest.fallback_route;
end;
$$;