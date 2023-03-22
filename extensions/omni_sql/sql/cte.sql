-- Append
select
    omni_sql.add_cte(
            omni_sql.add_cte('SELECT', 'a', 'SELECT 0'), 'b', 'SELECT 1');

-- Prepend
select
    omni_sql.add_cte(
            omni_sql.add_cte('SELECT', 'a', 'SELECT 0'), 'b', 'SELECT 1', prepend => true);

-- Recursive
select omni_sql.add_cte('SELECT', 'a', 'SELECT 0', recursive => true);

-- INSERT
select
    omni_sql.add_cte(
            omni_sql.add_cte('INSERT INTO t VALUES (1)', 'a', 'SELECT 0'), 'b', 'SELECT 1');

-- UPDATE
select
    omni_sql.add_cte(
            omni_sql.add_cte('UPDATE t SET a = 1', 'a', 'SELECT 0'), 'b', 'SELECT 1');

-- DELETE
select
    omni_sql.add_cte(
            omni_sql.add_cte('DELETE FROM t', 'a', 'SELECT 0'), 'b', 'SELECT 1');

