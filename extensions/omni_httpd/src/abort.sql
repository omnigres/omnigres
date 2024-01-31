create function abort() returns http_outcome as
$$
select omni_httpd.http_outcome_from_abort(omni_types.unit())
$$ language sql immutable;