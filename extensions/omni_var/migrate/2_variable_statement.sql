create function set_statement(name name, value anyelement) returns anyelement
    language c
as
'MODULE_PATHNAME';

create function get_statement(name name, default_value anyelement) returns anyelement
    language c
    stable
as
'MODULE_PATHNAME';

