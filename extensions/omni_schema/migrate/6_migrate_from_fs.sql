create function migrate_from_fs(fs anyelement, path text default '') returns setof text
    language plpgsql
as
$$
declare
    rec record;
begin
    for rec in select
                   case when path = '' then '' else path || '/' end || files.name      as name,
                   convert_from(omni_vfs.read(fs, path || '/' || files.name), 'utf-8') as code
               from
                   omni_vfs.list_recursively(fs, path, max => 10000) as files
                   left join omni_schema.migrations
                             on migrations.name = (case when path = '' then '' else path || '/' end || files.name)
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