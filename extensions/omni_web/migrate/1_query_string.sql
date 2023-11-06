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

create function param_get_all(params params, param text) returns setof text
    strict immutable
as
$$
select
    params[i + 1] as value
from
    unnest(params) with ordinality t(v, i)
where
    v = param and
    i % 2 = 1
order by
    i asc;
$$
    language sql;