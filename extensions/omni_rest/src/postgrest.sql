create procedure postgrest(request omni_httpd.http_request, response inout omni_httpd.http_outcome)
    language plpgsql
as
$$
declare
begin
    call omni_rest.postgrest_get(request, response);
    call omni_rest.postgrest_cors(request, response);
end;
$$;
