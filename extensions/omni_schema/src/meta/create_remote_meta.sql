create function create_remote_meta(
    schema regnamespace,
    remote_schema name,
    server name,
    materialize boolean default false) returns void
    language plpgsql
as
$create_remote_meta$
declare
    import_schema regnamespace := schema;
begin
    if materialize then
        declare
            import_schema_name text := '_' || (select nspname from pg_namespace where oid = schema) || '_foreign';
        begin
            execute format('create schema %I', import_schema_name);
            import_schema := import_schema_name::regnamespace;
        end;
    end if;
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

    if materialize then
        perform materialize_meta(import_schema, schema);
    end if;
    return;
end;
$create_remote_meta$;