create function http_header_get_all(headers http_headers, header_name text) returns setof text
    strict immutable
as
$$
select
    header.value
from
    unnest(headers) header(name, value)
where
    lower(header.name) = lower(header_name)
$$
    language sql;
