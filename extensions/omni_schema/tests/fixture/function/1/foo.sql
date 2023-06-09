create function foo() returns bool as
$$
select false
$$
    language sql;