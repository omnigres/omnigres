create function migrate_to_schema_revision(fs anyelement, revisions_path text,
                                           target revision_id, source_conn text,
                                           rollback boolean default true) returns jsonb
    language plpgsql
as
$$
declare
    host              text := ' host=' ||
                              current_setting('unix_socket_directories') || ' port=' ||
                              current_setting('port') ||
                              ' user=' || current_user;
    source_db         name;
    revision_metadata jsonb;
    parents           revision_id[];
    diff_schema       name;
    result            jsonb;
    conn              text := gen_random_uuid()::text;
begin
    select db into source_db from dblink(source_conn, 'select current_database()') t(db name);

    -- TODO:
    -- For now, we assume the case of direct Source->Target migration but we should be able to find
    -- the path and apply this function for all steps.

    if omni_vfs.file_info(fs, revisions_path || '/' || target || '/metadata.yaml') is not null then
        revision_metadata :=
                omni_yaml.to_json(convert_from(omni_vfs.read(fs, revisions_path || '/' || target || '/metadata.yaml'),
                                               'utf8'))::jsonb;
        parents := array((select jsonb_array_elements_text(revision_metadata -> 'parents')))::revision_id[];
    else
        raise exception 'Metadata file %s is not found', revisions_path || '/' || target || '/meta.yaml';
    end if;

    --- 1. Check for pre-conditions

    --- 1.0. Make sure source database has omni_schema and omni_schema.deployed_revision

    ---- Install omni_schema in the target schema to get its meta
    perform dblink(source_conn, format('create extension if not exists omni_schema version %L cascade',
                                       (select extversion from pg_extension where extname = 'omni_schema')));

    perform dblink(source_conn,
                   format('create table if not exists omni_schema.deployed_revision (revision omni_schema.revision_id)'));

    --- 1.1. The source database must be a parent of the target revision
    declare
        num_parents int;
    begin
        -- However, there's a case when this is the first revision and then everything is good
        if cardinality(parents) > 0 then
            select
                count
            into num_parents
            from
                dblink(source_conn,
                       format('select distinct count(*) from omni_schema.deployed_revision where revision = any (%L)',
                              parents)) t(count int);
            if num_parents != cardinality(parents) then
                raise exception '% database is not a parent to %', source_db, target;
            end if;
        end if;
    end;

    --- 2. Freeze source's schema meta

    ---- Create a foreign server to connect to the revision
    execute format(
            $sql$create server %1$I foreign data wrapper postgres_fdw options(host %2$L, dbname %1$L, port %3$L)$sql$,
            source_db, current_setting('unix_socket_directories'), current_setting('port'));
    execute format('create user mapping for %1$I server %2$I options (user %1$L)',
                   current_user, source_db);

    ---- Define remote meta
    execute format('create schema %I', 'current_revision');
    perform create_remote_meta('current_revision'::regnamespace, 'omni_schema'::name, source_db::text::name,
                               materialize => true);

    --- 3. Run necessary migrations
    perform dblink_connect(conn, source_conn);
    perform dblink(conn, 'begin');
    if cardinality(parents) = 0 then
        --- 3.0. Just assemble it if there are no parents (first migration)
        perform
        from
            assemble_schema_revision(fs, revisions_path, target, source_conn)
        where
            execution_error is not null;
        if found then
            raise exception 'Starting migration % cannot be assembled due to errors', current_parent
                using hint = 'Run assemble_schema to see errors';
        end if;
    else
        --- 3.1. Run migrate.sql file
        declare
            migration_path text;
            migration      text;
            rec            record;
        begin
            migration_path := revisions_path || '/' || target || '/migrate.sql';
            if omni_vfs.file_info(fs, migration_path) is not null then
                migration := convert_from(omni_vfs.read(fs, migration_path), 'utf8');
                for rec in select * from omni_sql.raw_statements(migration::cstring, true)
                    loop
                        case
                            when omni_sql.statement_type(rec.source::omni_sql.statement) = 'TransactionStmt'
                                then raise exception 'No transactional statements allowed' using detail =
                                        format('Line %s, Column %s: %s', rec.line, rec.col, rec.source);
                            when omni_sql.statement_type(rec.source::omni_sql.statement) = 'MultiStmt' and
                                 omni_sql.statement_type((select
                                                              array_agg(source order by ordinality)
                                                          from
                                                              omni_sql.raw_statements(rec.source::cstring) with ordinality
                                                          limit 1)[1]::omni_sql.statement) =
                                 'TransactionStmt'
                                then raise exception 'No transactional statements allowed' using detail =
                                        format('Line %s, Column %s: %s', rec.line, rec.col, rec.source);
                            else null;
                            end case;
                        begin
                            perform dblink(conn, rec.source);
                        exception
                            when others then
                                perform dblink(conn, 'rollback');
                                perform dblink_disconnect(conn);
                                raise using hint = rec.source;
                        end;
                    end loop;
            end if;
        end;
    end if;

    ---- Stamp new revision
    perform dblink_exec(source_conn,
                        format('insert into omni_schema.deployed_revision (revision) values (%L)',
                               target));

    --- 4. Compare diffs between the source database and the target revision, should be the same

    ---- Define remote meta
    execute format('create schema %I', target);
    perform create_remote_meta(target::text::regnamespace, 'omni_schema'::name, conn,
                               materialize => true);

    diff_schema := target::text || '_diff';
    execute format('create schema %I', diff_schema);
    perform create_meta_diff('current_revision'::regnamespace, target::text::regnamespace,
                             diff_schema::regnamespace);

    declare
        current_parent  revision_id;
        diff_path       text;
        diff            jsonb;
        expected_diff   jsonb := '{}';
        old_search_path text;
    begin
        foreach current_parent in array parents
            loop
                diff_path := revisions_path || '/' || target || '/' || current_parent || '.diffs.yaml';
                if omni_vfs.file_info(fs, diff_path) is not null then
                    diff := omni_yaml.to_json(convert_from(omni_vfs.read(fs, diff_path), 'utf8'))::jsonb;


                    old_search_path := current_setting('search_path');
                    perform set_config('search_path',
                                       diff_schema::regnamespace::text || ',' || old_search_path, true);


--## for direction in ["added","removed"]
--## for view in meta_views
                        expected_diff :=
                                jsonb_strip_nulls(jsonb_set(expected_diff, '{/*{{ direction }}*/_/*{{ view }}*/}',
                                                            coalesce((with
                                                                          t
                                                                              as materialized (select * from "/*{{ direction }}*/_/*{{ view }}*/")
                                                                      select
                                                                          jsonb_agg(to_jsonb(t))
                                                                      from
                                                                          t
                                                                          left join dependency d
                                                                                    on d.id = t.id::object_id and
                                                                                       omni_types.variant(d.dependent_on) =
                                                                                       'extension_id'::regtype
                                                                      where
                                                                          (d) is not distinct from null), 'null')));
--## endfor
--## endfor

                    perform set_config('search_path', old_search_path, true);

                    ---- Compare diffs. If they are not equal, we need to produce sets of differences for each
                    ---- provided type of diff.
                    if expected_diff != diff then
                        with
                            key_vals as (select
                                             coalesce(a.key, b.key) as key,
                                             a.value                as a_value,
                                             b.value                as b_value
                                         from
                                             jsonb_each(diff)                          a
                                             full outer join jsonb_each(expected_diff) b
                                                             on a.key = b.key)
                        select
                            jsonb_object_agg(key, difference)
                        into result
                        from
                            (select
                                 key,
                                 case
                                     -- If key exists only in j2, return its full array
                                     when a_value is null then b_value
                                     -- If key exists only in j1, return its full array
                                     when b_value is null then a_value
                                     else (
                                         -- For keys existing in both, compute the symmetric difference:
                                         select
                                             jsonb_agg(elem)
                                         from
                                             (
                                                 -- Elements in j1 not in j2
                                                 select
                                                     elem
                                                 from
                                                     jsonb_array_elements(a_value) elem
                                                 except
                                                 select
                                                     elem
                                                 from
                                                     jsonb_array_elements(b_value) elem
                                                 union
                                                 -- Elements in j2 not in j1
                                                 select
                                                     elem
                                                 from
                                                     jsonb_array_elements(b_value) elem
                                                 except
                                                 select
                                                     elem
                                                 from
                                                     jsonb_array_elements(a_value) elem) sub)
                                     end as difference
                             from
                                 key_vals
                             -- Only include keys that differ in some way:
                             where
                                 (a_value is not null and b_value is not null and
                                  a_value is distinct from b_value) or
                                 a_value is null or
                                 b_value is null) t;

                    end if;

                end if;
            end loop;
    end;

    if result is null then
        perform dblink(conn, 'commit');
        perform dblink_disconnect(conn);
    else
        raise notice 'Schema is not matching expectation, rolling back';
        perform dblink(conn, 'rollback');
        perform dblink_disconnect(conn);
    end if;

    --- 5. Compare the data with the target revision, revision's data should be present

    if rollback then
        raise exception 'migrate_to_schema_revision_done';
    end if;

    return result;

exception
    when others then
        if sqlerrm = 'migrate_to_schema_revision_done' then
            return result;
        else
            ---- re-raise otherwise
            raise;
        end if;

end;
$$;
