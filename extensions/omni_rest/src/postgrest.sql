create procedure postgrest(request omni_httpd.http_request, response inout omni_httpd.http_outcome,
                           settings postgrest_settings default postgrest_settings())
    language plpgsql
as
$$
declare
begin
    call omni_rest.postgrest_rpc(request, response, settings);
    call omni_rest.postgrest_get(request, response, settings);
    call omni_rest.postgrest_insert(request, response, settings);
    call omni_rest.postgrest_update(request, response, settings);
    call omni_rest.postgrest_cors(request, response, settings);
end;
$$;
