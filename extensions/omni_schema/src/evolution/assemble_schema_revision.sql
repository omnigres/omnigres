create function assemble_schema_revision(fs anyelement, revisions_path text, revision revision_id,
                                         db_conninfo text,
                                         rollback bool default true
)
    returns table
            (
                execution_position  int,
                migration_filename  text,
                migration_statement text,
                execution_error     text
            )
    language plpgsql
as
$$
declare
    db_sources jsonb;
    rec        record;
begin
    ---- Install omni_schema in the target schema to get its meta
    perform dblink(db_conninfo, format('create extension if not exists omni_schema version %L cascade',
                                       (select extversion from pg_extension where extname = 'omni_schema')));

    perform dblink(db_conninfo,
                   format(
                           'create table if not exists omni_schema.deployed_revision as select %L::omni_schema.revision_id as revision',
                           revision::text));


    db_sources =
            omni_yaml.to_json(convert_from(
                    omni_vfs.read(fs, revisions_path || '/' || revision || '/sources.yaml'), 'utf8'));

    for rec in with
                   db_fs as materialized (select omni_vfs.table_fs(revision::text) db_fs)
               select
                   db_fs.db_fs,
                   key,
                   decode(value #>> '{}', 'base64') as decoded
               from
                   db_fs
                   inner join (select * from jsonb_each(db_sources) as t(key, value)) files on true
        loop
            perform omni_vfs.write(rec.db_fs, rec.key, rec.decoded, create_file => true);
        end loop;

    perform from pg_tables where tablename = 'assembly_report' and schemaname like 'pg_temp_%';
    if found then
        drop table assembly_report;
    end if;
    create temporary table if not exists assembly_report on commit drop as
        (select *
         from
             assemble_schema(db_conninfo, omni_vfs.table_fs(revision::text), ''));

    for rec in select * from assembly_report
        loop
            execution_position := rec.execution_position;
            migration_filename := rec.migration_filename;
            migration_statement := rec.migration_statement;
            execution_error := rec.execution_error;
            return next;
            execution_position := null;
            migration_filename := null;
            migration_statement := null;
            execution_error := null;
        end loop;

    if rollback then
        raise exception 'assemble_schema_revision_done';
    end if;

    return;

exception
    when others then
        if sqlerrm = 'assemble_schema_revision_done' then
            return;
        else
            ---- re-raise otherwise
            raise;
        end if;
end;
$$;