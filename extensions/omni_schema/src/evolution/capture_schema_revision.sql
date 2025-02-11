create function capture_schema_revision(fs anyelement, source_path text, revisions_path text,
                                        rollback bool default true,
                                        parents revision_id[] default null) returns revision_id
    language plpgsql
as
$$
declare
    revision          revision_id       := uuidv7();
    host              text              := ' host=' ||
                                           current_setting('unix_socket_directories') || ' port=' ||
                                           current_setting('port') ||
                                           ' user=' || current_user;
    self_conn         text              := 'dbname=' || current_database() || host;
    revision_database text              := revision;
    revision_conninfo text              := 'dbname=' || revision_database || host;
    parent_databases  text[];
    parent_conninfo   text[];
    has_parents       bool              := coalesce(cardinality(parents), 0) > 0;
    revision_parents  revision_id[]     := coalesce(parents, '{}'); -- TODO: do we actually need to allow overriding?
    metadata          json;
    sources           jsonb;
    diffs             jsonb             := '{}';
    old_search_path   text;
    rec               record;
    --- We write to this fs first to ensure errors during capture don't leave
    --- us with a partially written capture
    transactional_fs  omni_vfs.table_fs := omni_vfs.table_fs(revision::text);
    captured_revisions jsonb;
begin
    --- 0. Prepare parents

    if parents is null then
        -- Find revisions that are not parents to any other revisions
        select
            coalesce(array_agg(r.revision), '{}')
        into revision_parents
        from
            schema_revisions(fs, revisions_path, leafs_only => true) r;
        has_parents := coalesce(cardinality(revision_parents), 0) > 0;
    end if;

    --- 1. Assemble current revision

    ---- Prepare the database in which we'll be assembling it. Make it a template database
    ---- so that services know it is not an operational database
    perform dblink(self_conn, format('create database %I', revision_database));
    update pg_database set datistemplate = true where datname = revision_database;

    ---- Install omni_schema in the target schema to get its meta
    perform dblink(revision_conninfo, format('create extension omni_schema version %L cascade',
                                             (select extversion from pg_extension where extname = 'omni_schema')));

    if not has_parents then
        ---- Create a foreign server to connect to the revision
        execute format(
                $sql$create server %1$I foreign data wrapper postgres_fdw options(host %2$L, dbname %4$L, port %3$L)$sql$,
                revision_database || '_empty', current_setting('unix_socket_directories'), current_setting('port'),
                revision_database);
        execute format('create user mapping for %1$I server %2$I options (user %1$L)',
                       current_user, revision_database || '_empty');
        ---- Define remote meta
        execute format('create schema %I', revision || '_empty');
        perform create_remote_meta((revision::text || '_empty')::regnamespace, 'omni_schema'::name,
                                   revision::text || '_empty'::name, materialize => true);

        -- "Freeze" empty meta for further comparison. This is done by issuing a single
        -- query that would put the connection into the repeatable read mode (as per postgres_fdw
        -- implementation), letting us see this snapshot on this connection.
        execute format('select * from %I.relation', revision || '_empty');
    end if;

    perform dblink(revision_conninfo,
                   format(
                           'create table omni_schema.deployed_revision as select %L::omni_schema.revision_id as revision',
                           revision::text));

    perform from assemble_schema(revision_conninfo, fs, source_path) where execution_error is not null;
    if found then
        raise exception 'New revision cannot be assembled due to errors' using hint = 'Run assemble_schema to see errors';
    end if;

    --- 2. Assemble parents (if any)

    declare
        current_parent  revision_id;
        parent_conninfo text;
        parent_sources  jsonb;
        rec             record;
    begin
        foreach current_parent in array revision_parents
            loop
            ---- Prepare the database in which we'll be assembling it. Make it a template database
            ---- so that services know it is not an operational database
                perform dblink(self_conn, format('drop database if exists %I', current_parent::text));
                perform dblink(self_conn, format('create database %I', current_parent::text));
                update pg_database set datistemplate = true where datname = current_parent::text;

                parent_conninfo := 'dbname=' || current_parent || host;

                perform
                from
                    assemble_schema_revision(fs, revisions_path, current_parent, parent_conninfo)
                where
                    execution_error is not null;
                if found then
                    raise exception 'Parent % cannot be assembled due to errors', current_parent
                        using hint = 'Run assemble_schema to see errors';
                end if;
                update pg_database set datistemplate = false where datname = current_parent::text;
            end loop;
    end;

    --- 3. Write metadata
    metadata := json_build_object('parents', revision_parents);
    perform omni_vfs.write(transactional_fs, revision || '/metadata.yaml',
                           convert_to(omni_yaml.to_yaml(metadata), 'utf8'), create_file => true);

    --- 4. Capture current source

    ---- FIXME: saving to YAML for now just to make _a_ decision
    select
        jsonb_object_agg(name, encode(omni_vfs.read(fs, source_path || '/' || name), 'base64'))
    into sources
    from
        omni_vfs.list_recursively(fs, source_path)
    where
        kind = 'file';

    perform omni_vfs.write(transactional_fs, revision || '/sources.yaml',
                           convert_to(omni_yaml.to_yaml(sources::json), 'utf8'), create_file => true);

    --- 5. Capture diffs

    ---- Create a foreign server to connect to the revision
    execute format(
            $sql$create server %1$I foreign data wrapper postgres_fdw options(host %2$L, dbname %1$L, port %3$L)$sql$,
            revision_database, current_setting('unix_socket_directories'), current_setting('port'));
    execute format('create user mapping for %1$I server %2$I options (user %1$L)',
                   current_user, revision_database);

    ---- Define remote meta
    execute format('create schema %I', revision);
    perform create_remote_meta(revision::text::regnamespace, 'omni_schema'::name, revision::text::name,
                               materialize => true);

    --- If no parents, compare with the empty version
    if not has_parents then

        execute format('create schema %I', revision || '_diff');
        perform create_meta_diff((revision || '_empty')::regnamespace, revision::text::regnamespace,
                                 (revision || '_diff')::regnamespace);

        old_search_path := current_setting('search_path');
        perform set_config('search_path',
                           (revision::text || '_diff')::regnamespace || ',' || revision::text::regnamespace || ',' ||
                           old_search_path, true);

--## for direction in ["added","removed"]
--## for view in meta_views
        diffs := jsonb_strip_nulls(jsonb_set(diffs, '{/*{{ direction }}*/_/*{{ view }}*/}',
                                             coalesce((with
                                                           t
                                                               as materialized (select * from "/*{{ direction }}*/_/*{{ view }}*/")
                                                       select
                                                           jsonb_agg(to_jsonb(t))
                                                       from
                                                           t
                                                           left join dependency d on d.id = t.id::object_id and
                                                                                     omni_types.variant(d.dependent_on) =
                                                                                     'extension_id'::regtype
                                                       where
                                                           (d) is not distinct from null), 'null')));
--## endfor
--## endfor

        perform set_config('search_path', old_search_path, true);

        perform omni_vfs.write(transactional_fs, revision || '/diffs.yaml',
                               convert_to(omni_yaml.to_yaml(diffs::json), 'utf8'), create_file => true);

    else
        --- Otherwise, compare with each parent
        declare
            current_parent revision_id;
            diff_schema    text;
        begin

            foreach current_parent in array revision_parents
                loop

                    ---- Create a foreign server to connect to the parent
                    execute format(
                            $sql$create server %1$I foreign data wrapper postgres_fdw options(host %2$L, dbname %1$L, port %3$L)$sql$,
                            current_parent, current_setting('unix_socket_directories'), current_setting('port'));
                    execute format('create user mapping for %1$I server %2$I options (user %1$L)',
                                   current_user, current_parent);

                    ---- Define remote meta
                    execute format('create schema %I', current_parent);
                    perform create_remote_meta(current_parent::text::regnamespace, 'omni_schema'::name,
                                               current_parent::text::name, materialize => true);


                    diff_schema := current_parent::text || '_diff';
                    execute format('create schema %I', diff_schema);
                    perform create_meta_diff(current_parent::text::regnamespace, revision::text::regnamespace,
                                             diff_schema::regnamespace);

                    old_search_path := current_setting('search_path');
                    perform set_config('search_path',
                                       diff_schema::regnamespace || ',' || old_search_path, true);

--## for direction in ["added","removed"]
--## for view in meta_views
                    diffs := jsonb_strip_nulls(jsonb_set(diffs, '{/*{{ direction }}*/_/*{{ view }}*/}',
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

                    perform omni_vfs.write(transactional_fs,
                                           revision || '/' || current_parent || '.diffs.yaml',
                                           convert_to(omni_yaml.to_yaml(diffs::json), 'utf8'), create_file => true);
                end loop;
        end;
    end if;


    --- 6. Prepare a migration boilerplate?
    --- 7. Transformation functions?
    --- 8. Capture data

    old_search_path := current_setting('search_path');
    perform set_config('search_path', (revision::text)::regnamespace || ',' || old_search_path, true);

    for rec in
        with
            tables as materialized (select
                                        schema_name,
                                        name
                                    from
                                        "table"                            t
                                        natural inner join table_permanent tp),
            data as materialized (select
                                      schema_name,
                                      name,
                                      result as json
                                  from
                                      tables
                                      inner join lateral (select *
                                                          from
                                                              dblink(revision_conninfo,
                                                                     format('select json_agg(to_json(t)) from %I.%I t', schema_name, name)) t(result json)) t
                                                 on true
                                  where
                                      result is not null)
        select *
        from
            data
        where
            schema_name != 'pg_catalog' and
            schema_name != 'information_schema'
        loop
            perform omni_vfs.write(transactional_fs,
                                   revision || '/data/' || rec.schema_name || '.' || rec.name || '.yaml',
                                   convert_to(omni_yaml.to_yaml(rec.json), 'utf8'), create_file => true);
        end loop;


    perform set_config('search_path', old_search_path, true);

    --- X. Done

    ---- Prepare to commit transactional_fs back to the fs
    select
        jsonb_object_agg(name, encode(coalesce(omni_vfs.read(transactional_fs, name), ''), 'base64'))
    into captured_revisions
    from
        omni_vfs.list_recursively(transactional_fs, '')
    where kind = 'file';

    ---- Clean up
    ---- FIXME: can't really do this, even though the foreign servers are gone,
    ----        but the connections won't be released until the very end of the
    ----        transaction. For now, this has to be cleaned up manually.
    ---- perform dblink(self_conn, format('drop database %I', revision_database));
    update pg_database set datistemplate = false where datname = revision_database;
    raise notice '%', format('Target database %I remained', revision_database)
        using
            detail =
                    'Due to limitations of postgres_fdw transactional scope, it cannot be dropped from within this function',
            hint = format('Remove it with `drop database %I`', revision_database);
    declare
        current_parent revision_id;
    begin
        foreach current_parent in array revision_parents
            loop
                raise notice '%', format('Target database %I remained', current_parent)
                    using
                        detail =
                                'Due to limitations of postgres_fdw transactional scope, it cannot be dropped from within this function',
                        hint = format('Remove it with `drop database %I`', current_parent);

            end loop;
    end;

    if rollback then
        raise exception 'capture_schema_revision_done';
    end if;

    return revision;

exception
    when others then
        if sqlerrm = 'capture_schema_revision_done' then
            ---- Commit transactional_fs back to the fs
            perform omni_vfs.write(fs, revisions_path || '/' || key,
                                   decode(value #>> '{}', 'base64'),
                                   create_file => true)
            from
                jsonb_each(captured_revisions) t;
            ---- Return the revision ID if this is just the [default] rollback behavior,
            return revision;
        else
            ---- re-raise otherwise
            raise;
        end if;
end;
$$;
