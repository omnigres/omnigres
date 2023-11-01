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

create function param_get_all(params text, param text) returns setof text
    strict immutable
as
$$
select omni_web.param_get_all(omni_web.parse_query_string(params), param);
$$
    language sql;

create function param_get_all(params bytea, param text) returns setof text
    strict immutable
as
$$
select omni_web.param_get_all(omni_web.parse_query_string(convert_from(params, 'UTF8')), param);
$$
    language sql;

create function param_get(params params, param text) returns text
    strict immutable
as
$$
select
    param_get_all
from
    omni_web.param_get_all(params, param)
limit 1
$$
    language sql;

create function param_get(params text, param text) returns text
    strict immutable
as
$$
select omni_web.param_get(omni_web.parse_query_string(params), param);
$$
    language sql;

create function param_get(params bytea, param text) returns text
    strict immutable
as
$$
select omni_web.param_get(omni_web.parse_query_string(convert_from(params, 'UTF8')), param);
$$
    language sql;

create function cookies(cookies text)
    returns table
            (
                name  text,
                value text
            )
    language sql
as
$$
select
    split_part(cookie, '=', 1),
    split_part(cookie, '=', 2)
from
    unnest(string_to_array(cookies, '; ')) cookies_arr(cookie);
$$;

create function url_encode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;
create function url_decode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;

create function uri_encode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;
create function uri_decode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;