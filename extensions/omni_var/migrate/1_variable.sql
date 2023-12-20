create function set(name name, value anyelement) returns anyelement
    language c
as 'MODULE_PATHNAME';

create function get(name name, default_value anyelement) returns anyelement
    language c
    stable
as
'MODULE_PATHNAME';

