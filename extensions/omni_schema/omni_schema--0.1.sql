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

create function load_from_fs(fs anyelement, path text) returns setof text
    language plpgsql
as
$$
declare
    rec record;
begin
    -- Procs
    create temporary table if not exists _omni_schema_pg_proc on commit drop as
        select * from pg_proc;
    for rec in select * from omni_schema.procs
        loop
            execute format('drop function if exists %s', rec.oid::regprocedure);
        end loop;
    delete from omni_schema.procs;
    -- Policies
    create temporary table if not exists _omni_schema_pg_policy on commit drop as
        select * from pg_policy;
    for rec in select
                   policies.*,
                   pg_class.relname
               from
                   omni_schema.policies
                   inner join pg_class on pg_class.oid = policies.polrelid
        loop
            execute format('drop policy if exists %I on %I', rec.polname, rec.relname);
        end loop;
    delete from omni_schema.policies;
    -- Execute
    for rec in select
                   path || '/' || name                                           as name,
                   convert_from(omni_vfs.read(fs, path || '/' || name), 'utf-8') as code
               from
                   omni_vfs.list_recursively(fs, path, max => 10000)
               where
                   name like '%.sql'
        loop
            execute rec.code;
            return next rec.name;
        end loop;
    -- New procs
    insert
    into
        omni_schema.procs (select * from pg_proc except select * from _omni_schema_pg_proc);
    -- New policies
    insert
    into
        omni_schema.policies (select * from pg_policy except select * from _omni_schema_pg_policy);
    return;
end ;
$$;

--- Migrations
create table migrations
(
    id         integer primary key generated always as identity,
    name       text      not null,
    migration  text      not null,
    applied_at timestamp not null default clock_timestamp()
);

select pg_catalog.pg_extension_config_dump('migrations', '');
select pg_catalog.pg_extension_config_dump('migrations_id_seq', '');

create function migrate_from_fs(fs anyelement, path text) returns setof text
    language plpgsql
as
$$
declare
    rec record;
begin
    for rec in select
                   path || '/' || files.name                                           as name,
                   convert_from(omni_vfs.read(fs, path || '/' || files.name), 'utf-8') as code
               from
                   omni_vfs.list_recursively(fs, path, max => 10000) as files
                   left join omni_schema.migrations on migrations.name = (path || '/' || files.name)
               where
                   files.name like '%.sql' and
                   migrations.name is null
               order by files.name asc
        loop
            execute rec.code;
            return next rec.name;
            insert
            into
                omni_schema.migrations (name, migration)
            values (rec.name, rec.code);
        end loop;
    return;
end;
$$;