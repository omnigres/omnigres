create function http_request(path text, method omni_http.http_method default 'GET', query_string text default null,
                             body bytea default null,
                             headers http_headers default array []::http_headers)
    returns http_request
    language sql
    immutable
as
$$
select row (method, path, query_string, body, headers)
$$;