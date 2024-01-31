create function create_functions(code text, filename text default null, replace boolean default false,
                                 fs anyelement default null::bool) returns setof regprocedure
    language plpgsql
as
$$
declare
    rec  record;
    args text;
    fun  regprocedure;
begin
    lock table pg_proc in access exclusive mode;
    for rec in select *
               from omni_python.functions(code => code, filename => filename, fs => fs::text, fs_type => pg_typeof(fs))
        loop
            select
                array_to_string(array_agg(name || ' ' || type), ', ')
            from
                (select * from unnest(rec.argnames, rec.argtypes) as args(name, type)) a
            into args;
            create temporary table current_procs as (select * from pg_proc);
            execute format('create %s function %I(%s) returns %s language plpython3u as %L',
                           (case when replace then 'or replace' else '' end),
                           rec.name, args, rec.rettype, rec.code);
            select
                pg_proc.oid::regprocedure
            into fun
            from
                pg_proc
                left join current_procs on current_procs.oid = pg_proc.oid
            where
                current_procs.oid is null;
            return next fun;
            drop table current_procs;
        end loop;
    return;
end;
$$;