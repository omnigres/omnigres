-- Replace function with procedure
drop function handler(int, http_request);

-- Empty implementation of the handler
-- NOTE: we use `inout` here mostly to support Postgres 13
-- (Postgres 13 does not support `out` in procedures)
-- and we're currently passing `null` into it
-- TODO: review this policy once we stop supporting Postgres 13
create procedure handler(int, http_request, inout http_outcome)
    language plpgsql as
$$
begin
    $3 := omni_httpd.http_response(status => 404);
end;
$$;

comment on procedure handler(int, http_request, inout http_outcome) is $$
This procedure handlers incoming HTTP requests. Can't be deleted but can be replaced.
$$;
