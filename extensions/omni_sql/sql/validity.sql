-- Valid
SELECT omni_sql.is_valid('SELECT'::omni_sql.statement);

-- An invalid table
SELECT omni_sql.is_valid('SELECT * FROM no_such_table_exists'::omni_sql.statement);

-- Parameters
SELECT omni_sql.is_valid('SELECT $1'::omni_sql.statement);

-- Multiple statements (one is invalid)
SELECT omni_sql.is_valid('SELECT; SELECT $1'::omni_sql.statement);
