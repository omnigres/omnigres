create function is_returning_statement(statement) returns boolean
as
'MODULE_PATHNAME' language c strict
  immutable;


comment on function omni_sql.is_returning_statement is 'Does the statement returns rows? In other words, can a cursor(portal more precisely) be created?';
