create function postgrest_cors_route(request omni_httpd.http_request) returns postgrest_cors_route
    language plpgsql
as
$$
declare
    origin text;
begin
    if request.method = 'OPTIONS' then
        origin := omni_http.http_header_get(request.headers, 'origin');
        return
            row (request, origin)::omni_rest.postgrest_cors_route;
    end if;
    return null;
end;
$$;