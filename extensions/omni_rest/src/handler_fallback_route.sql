create function handler(fallback_route) returns omni_httpd.http_outcome
    strict
    language plpgsql as
$$
begin
    return omni_httpd.http_response(status => 404);
end;
$$;