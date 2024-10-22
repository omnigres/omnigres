create function postgrest(request omni_httpd.http_request) returns omni_httpd.http_outcome
    language plpgsql
as
$$
declare
begin
    raise notice '%',request;
    return coalesce(omni_rest.handler(omni_rest.postgrest_get_route(request)),
                    omni_rest.handler(omni_rest.postgrest_cors_route(request))
           );
end;
$$;
