create type raw_statement as
(
    source text,
    line   int,
    col    int
);

create function raw_statements(cstring) returns setof raw_statement
as
'MODULE_PATHNAME'
    language c strict
               immutable;