create type statement;

create function statement_in(cstring) returns statement
as
'MODULE_PATHNAME',
'statement_in'
    language c strict
               immutable;

create function statement_out(statement) returns cstring
as
'MODULE_PATHNAME',
'statement_out'
    language c strict
               immutable;

create type statement
(
    input = statement_in,
    output = statement_out,
    like = text
);

create function is_parameterized(statement) returns bool
as
'MODULE_PATHNAME',
'is_parameterized'
    language c strict
               immutable;

create function is_valid(statement) returns bool
as
'MODULE_PATHNAME',
'is_valid'
    language c strict
               immutable;
