 -- Append
SELECT omni_sql.add_cte(
    omni_sql.add_cte('SELECT', 'a', 'SELECT 0'), 'b', 'SELECT 1');

-- Prepend
SELECT omni_sql.add_cte(
    omni_sql.add_cte('SELECT', 'a', 'SELECT 0'), 'b', 'SELECT 1', prepend => true);

 -- Recursive
 SELECT omni_sql.add_cte('SELECT', 'a', 'SELECT 0', recursive => true);

-- INSERT
SELECT omni_sql.add_cte(
    omni_sql.add_cte('INSERT INTO t VALUES (1)', 'a', 'SELECT 0'), 'b', 'SELECT 1');

-- UPDATE
SELECT omni_sql.add_cte(
    omni_sql.add_cte('UPDATE t SET a = 1', 'a', 'SELECT 0'), 'b', 'SELECT 1');

-- DELETE
SELECT omni_sql.add_cte(
    omni_sql.add_cte('DELETE FROM t', 'a', 'SELECT 0'), 'b', 'SELECT 1');

