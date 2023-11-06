create table procs as
    select *
    from
        pg_proc
    limit 0;

create table policies as
    select *
    from
        pg_policy
    limit 0;

create table class as
    select *
    from
        pg_class
    limit 0;