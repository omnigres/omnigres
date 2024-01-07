create function http_header_get(headers http_headers, name text) returns text
    strict immutable
as
$$
select
    http_header_get_all
from
    http_header_get_all(headers, name)
limit 1
$$
    set search_path to '@extschema@'
    language sql;
