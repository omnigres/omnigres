create function set_variable(name name, value anyelement) returns anyelement language c
as 'MODULE_PATHNAME';

create function get_variable(name name, default_value anyelement) returns anyelement
    language c
    stable
as
'MODULE_PATHNAME';

