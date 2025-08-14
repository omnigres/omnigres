create function dump(connstr text, schema bool default true, data bool default true) returns text
    security definer
    language plpgsql
as
$$
declare
    bindir text;
    dump   text;
    opts text := '';
begin
    begin
        perform dblink_connect('dump_' || connstr, connstr);
        perform dblink_disconnect('dump_' || connstr);
    exception
        when others then
            raise exception 'Invalid connection string: %', sqlerrm;
    end;

    case
        when data and not schema then opts := '-a';
        when not data and schema then opts := '-s';
        when data and schema then null;
        when not data and not schema then raise exception 'either schema or data must be included';
        end case;

    select setting from pg_config() where name = 'BINDIR' into bindir;
    create temporary table _pg_dump
    (
        value text
    ) on commit drop;
    execute format(
            $copy$copy _pg_dump from program $p$%L/pg_dump %s %L$p$ delimiter E'\x1E' $copy$,
            bindir,
            opts,
            connstr);
    select string_agg(value, '\n') from _pg_dump into dump;
    return dump;
end;
$$;
