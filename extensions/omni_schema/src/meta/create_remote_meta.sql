create function create_remote_meta(
    schema regnamespace,
    remote_schema name,
    server name) returns void
    language plpgsql
as
$create_remote_meta$
declare
begin
    execute format(
        $import$
            import foreign schema %I limit to (
--## for view in meta_views
            "/*{{ view }}*/"
--## endfor
           ) from server %I into %I
        $import$,
        remote_schema, server, schema);
    return;
end;
$create_remote_meta$;