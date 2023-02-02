 -- Append
SELECT omni_sql.add_cte(
    omni_sql.add_cte('SELECT'::omni_sql.statement, 'a', 'SELECT 0'::omni_sql.statement), 'b', 'SELECT 1');

-- Prepend
SELECT omni_sql.add_cte(
    omni_sql.add_cte('SELECT'::omni_sql.statement, 'a', 'SELECT 0'::omni_sql.statement), 'b', 'SELECT 1', prepend => true);

 -- Recursive
 SELECT omni_sql.add_cte('SELECT'::omni_sql.statement, 'a', 'SELECT 0'::omni_sql.statement, recursive => true);

-- INSERT
SELECT omni_sql.add_cte(
    omni_sql.add_cte('INSERT INTO t VALUES (1)'::omni_sql.statement, 'a', 'SELECT 0'::omni_sql.statement), 'b', 'SELECT 1');

-- UPDATE
SELECT omni_sql.add_cte(
    omni_sql.add_cte('UPDATE t SET a = 1'::omni_sql.statement, 'a', 'SELECT 0'::omni_sql.statement), 'b', 'SELECT 1');

-- DELETE
SELECT omni_sql.add_cte(
    omni_sql.add_cte('DELETE FROM t'::omni_sql.statement, 'a', 'SELECT 0'::omni_sql.statement), 'b', 'SELECT 1');

