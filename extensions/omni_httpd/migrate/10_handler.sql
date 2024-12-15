-- Replace function with procedure
drop function handler(int, http_request);

-- Empty implementation
create procedure handler(int, http_request, out http_outcome)
    language plpgsql as
$$
begin
    $3 := omni_httpd.http_response(status => 404);
end;
$$;

comment on procedure handler(int, http_request, out http_outcome) is $$
This procedure handlers incoming HTTP requests. Can't be deleted but can be replaced.
$$;
