create function parse_query_string(query_string text) returns text[]
    strict immutable
as
'MODULE_PATHNAME',
'parse_query_string' language c;