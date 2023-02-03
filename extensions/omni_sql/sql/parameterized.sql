SELECT omni_sql.is_parameterized('SELECT 1'::omni_sql.statement);

SELECT omni_sql.is_parameterized('SELECT $1'::omni_sql.statement);