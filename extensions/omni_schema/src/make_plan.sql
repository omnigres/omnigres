create or replace function make_plan(path text) 
    returns table
            (
                migration_filename  text,
                migration_statement omni_sql.statement
            )
    language plpgsql
as
$$
declare
    local_conn_info text := 'host=localhost port=' || (select setting from pg_settings where name = 'port') || ' user=' || current_user;
    current_db_conn text := local_conn_info || ' dbname=' || current_database();
    plan_db text := 'plan_' || gen_random_uuid();
    plan_db_conn text := local_conn_info || ' dbname=' || plan_db;
    migration record;
    stmt omni_sql.statement;
    error_message text;
    hint_message text;
    extension_record record;
begin
    perform dblink(current_db_conn, format('create database %I', plan_db));
    perform dblink(plan_db_conn, 'create extension omni_schema cascade;');

    -- prepare necessary extensions in the remote database
    for extension_record in select distinct extension
               from omni_schema.languages
               union
               select distinct file_processor_extension as extension
               from omni_schema.languages
               union
               select distinct processor_extension as extension
               from omni_schema.auxiliary_tools
    loop
            if exists (select from pg_extension where extname = extension_record.extension) then
                perform dblink(plan_db_conn,
                               format('create extension if not exists %s cascade', extension_record.extension));
            end if;
    end loop;

    for migration in select * from dblink(
        plan_db_conn,
        format('select migration_filename, executed_statement, migration_statement, execution_error from omni_schema.assemble_schema(%L, omni_vfs.local_fs(%L))', plan_db_conn, path)
    ) migrations (migration_filename text, executed_statement text, migration_statement text, execution_error text)
    loop
        -- first we have to handle errors coming from assemble_schema
        if migration.executed_statement is not null and migration.execution_error is null then
            begin
                stmt := migration.executed_statement::omni_sql.statement;
            exception
                when others then
                    get stacked diagnostics error_message = message_text,
                                  hint_message = pg_exception_hint;
                    raise exception 'Error "%" in %', error_message, migration.migration_filename
                        using 
                            detail = migration.executed_statement,
                            hint = hint_message;
            end;
        elsif migration.execution_error is not null then
            raise exception '% in %', migration.execution_error, migration.migration_filename
                using 
                    detail = migration.migration_statement;
        end if;

        -- now we branch between 2 execution modes: migration or source
        if migration.migration_filename ~* 'migrations' then
            -- migrations always execute only once
            if not exists(select from omni_schema.migrations m where lower(m.name) = lower(omni_vfs.basename(migration.migration_filename))) 
            then
                return query select migration.migration_filename, migration.executed_statement::omni_sql.statement;
            end if;
        else
            -- Non-migrations are picky, we error unless conditions are met
            if not (
                migration.executed_statement ~* 'select omni_python.install_requirements'
                or omni_sql.is_replace_statement(migration.executed_statement::omni_sql.statement) 
                or omni_sql.statement_type(migration.executed_statement::omni_sql.statement) = 'CreatePolicyStmt'
            )
            then
                raise exception 'Non replace statement found outside migrations directory in %', migration.migration_filename
                    using 
                        detail = migration.executed_statement,
                        hint = 'Move file into migrations or ensure it contains only statements with OR REPLACE';
            end if;

            -- policies have to be dropped first
            declare
                drop_stmt text;
            begin
                if omni_sql.statement_type(migration.executed_statement::omni_sql.statement) = 'CreatePolicyStmt' then
                    with m as (
                        select regexp_match(
                            migration.executed_statement,
                            'CREATE POLICY (.*) ON (.*) TO '
                        )
                    ) 
                    select format(
                        'DROP POLICY IF EXISTS %s ON %s',
                        m.regexp_match[1],
                        m.regexp_match[2]
                    ) from m into drop_stmt;

                    return query select migration.migration_filename, drop_stmt::omni_sql.statement;
                end if;

            end;

            -- non-migration always execute
            return query select migration.migration_filename, migration.executed_statement::omni_sql.statement;
        end if;

    end loop;

    -- PG 15 & 16 have issues with DROP DATABASE over dblink 
    -- https://www.postgresql.org/message-id/20231122012945.74%40rfd.leadboat.com
    if current_setting('server_version') ~ '(15|16).*' then
        perform dblink_connect(plan_db, plan_db_conn);
        perform dblink_send_query(plan_db, format('drop database if exists %I', plan_db));
        while dblink_is_busy(plan_db)
            loop
                null;
            end loop;
        perform dblink_disconnect(plan_db);
    else
        perform dblink(current_db_conn, format('drop database if exists %I', plan_db));
    end if;

    return;
end ;
$$;
