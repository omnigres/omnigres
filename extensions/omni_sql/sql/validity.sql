-- Valid
select omni_sql.is_valid('SELECT'::omni_sql.statement);

-- An invalid table
select omni_sql.is_valid('SELECT * FROM no_such_table_exists'::omni_sql.statement);

-- Parameters
select omni_sql.is_valid('SELECT $1'::omni_sql.statement);

-- Multiple statements (one is invalid)
select omni_sql.is_valid('SELECT; SELECT $1'::omni_sql.statement);
