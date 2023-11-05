create function cascading_query_reduce(internal, name text, query text) returns internal
as
'MODULE_PATHNAME',
'cascading_query_reduce' language c;

create function cascading_query_final(internal) returns text
as
'MODULE_PATHNAME',
'cascading_query_final' language c;

create aggregate cascading_query (name text, query text) (
    sfunc = cascading_query_reduce,
    finalfunc = cascading_query_final,
    stype = internal
    );
