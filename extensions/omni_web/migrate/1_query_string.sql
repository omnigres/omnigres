create domain params as text[] check (array_length(value, 1) % 2 = 0);

create function parse_query_string(query_string text) returns params
    strict immutable
as
'MODULE_PATHNAME',
'parse_query_string' language c;

create function parse_query_string(query_string bytea) returns params
    strict immutable
as
$$
select omni_web.parse_query_string(convert_from(query_string, 'UTF8'));
$$ language sql;