create function http_header(name text, value text) returns http_header as
$$
select row (name, value) as result;
$$
    language sql;