create or replace function make(path text) 
    returns table
            (
                migration_filename  text,
                migration_statement omni_sql.statement
            )
    language plpgsql
as
$$
declare
    migration record;
    error_message text;
    hint_message text;
begin
    for migration in select * from omni_schema.make_plan(path)
    loop
        begin
            execute migration.migration_statement;
        exception
            when others then
                get stacked diagnostics error_message = message_text,
                              hint_message = pg_exception_hint;
                raise exception 'Error "%" in %', error_message, migration.migration_filename
                    using 
                        detail = migration.migration_statement,
                        hint = hint_message;
        end;

        insert into omni_schema.migrations (name, migration, applied_at) values (omni_vfs.basename(migration.migration_filename), migration.migration_statement, current_timestamp);
        return query select migration.migration_filename, migration.migration_statement;
    end loop;
    return;
end ;
$$;
