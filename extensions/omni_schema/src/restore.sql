create function restore(connstr text, schema text) returns void
    security definer
    language plpgsql
as
$$
declare
    bindir text;
    dump   text;
begin
    begin
        perform dblink_connect('restore_dump_' || connstr, connstr);
        perform dblink_disconnect('restore_dump_' || connstr);
    exception
        when others then
            raise exception 'Invalid connection string: %', sqlerrm;
    end;
    select setting from pg_config() where name = 'BINDIR' into bindir;
    create temporary table _pg_restore
    (
        value text
    ) on commit drop;
    insert into _pg_restore select v from unnest(string_to_array(schema, E'\\n')) v;
    execute format(
            $copy$copy _pg_restore to program $p$%L/psql %L$p$ $copy$,
            bindir,
            connstr);
    return;
end;
$$;
