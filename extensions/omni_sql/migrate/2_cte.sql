create function add_cte(statement, name text, cte statement,
                        recursive bool default false, prepend bool default false) returns statement
as
'MODULE_PATHNAME',
'add_cte'
    language c strict
               immutable;

