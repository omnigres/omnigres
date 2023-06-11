create function foo(integer) returns bool as
$$ select $1 > 0 $$
    language sql;