create function is_replace_statement(statement) returns boolean
as
'MODULE_PATHNAME' language c strict
  immutable;


 comment on function omni_sql.is_replace_statement is 'Does the statement contain a OR REPLACE clause?';
