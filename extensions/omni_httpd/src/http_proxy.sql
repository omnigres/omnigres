create function http_proxy(url text, preserve_host boolean default true) returns http_outcome as
$$
select omni_httpd.http_outcome_from_http_proxy(row (url, preserve_host))
$$ language sql immutable;