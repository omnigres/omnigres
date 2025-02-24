create function explain(statement) returns json
as
'MODULE_PATHNAME',
'explain'
    language c strict
               immutable;
