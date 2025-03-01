create function create_remote_meta(
    schema regnamespace,
    remote_schema name,
    server name,
    materialize boolean default false,
    dblink_preferred boolean default false) returns void
    language plpgsql
as
$create_remote_meta$
declare
    import_schema   regnamespace := schema;
    old_search_path text         := current_setting('search_path');
begin
    if materialize then
        declare
            import_schema_name text := '_' || (select nspname from pg_namespace where oid = schema) || '_foreign';
        begin
            execute format('create schema %I', import_schema_name);
            import_schema := import_schema_name::regnamespace;
        end;
    end if;

    perform from pg_foreign_server where srvname = server and not dblink_preferred;
    if not found then
        -- Attempt to see if this is a dblink connection
        if coalesce(server = any (dblink_get_connections()), false) then
            if not materialize then
                raise exception 'dblink remote meta can only be materialized' using detail = 'This is because dblink is query execution oriented';
            end if;
            perform set_config('search_path', import_schema || ',public', true);
--## for view in meta_views

            declare
                signature text;
            begin
                select
                    string_agg(quote_ident(rc.name) || ' ' ||
                              -- FIXME: we are not quoting type name here (vv) because it may contain subscript for array types
                              (quote_ident(type.schema_name) || '.' || type.name), ', ' order by rcp.position)
                into signature
                from
                    omni_schema.relation_column                   rc
                    natural join omni_schema.relation_column_type rct
                    natural join omni_schema.relation_column_position rcp
                    inner join   omni_schema.type on type.id = type_id
                where
                    relation_id = omni_schema.relation_id('omni_schema', '/*{{ view }}*/');

                execute format('create view %1$I as select * from public.dblink(%2$L, ''select * from %3$I.%1$I'') as t(%4$s)', '/*{{ view }}*/',  server, remote_schema, signature);
            end;
--## endfor
            perform set_config('search_path', old_search_path, true);
        else
            raise exception 'no foreign server or dblink named % found', server;
        end if;
    else
        execute format(
                $import$
            import foreign schema %I limit to (
--## for view in meta_views
            "/*{{ view }}*/"
--## if not loop.is_last
            ,
--## endif
--## endfor
           ) from server %I into %s
        $import$,
                remote_schema, server, import_schema);

    end if;
    if materialize then
        -- FIXME: hard-coded schema here
        perform omni_schema.materialize_meta(import_schema, schema);
    end if;
    return;
end;
$create_remote_meta$;
