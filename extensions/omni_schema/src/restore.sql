create function _restore(connstr text, schema text) returns void
    security definer
    set search_path = pg_temp, public
    language plpgsql
as
$$
declare
    bindir text;
    dump   text;
begin
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

create function restore(connstr text, schema text) returns void
    language plpgsql
as
$$
begin
    perform omni_schema._validate_connstr(connstr);

    perform omni_schema._restore(connstr, schema);
    return;
end;
$$;
