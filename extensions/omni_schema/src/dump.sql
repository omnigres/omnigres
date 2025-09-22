create function _dump(connstr text, schema bool default true, data bool default true) returns text
    security definer
    set search_path = pg_temp, public
    language plpgsql
as
$$
declare
    bindir text;
    dump   text;
    opts   text := '';
begin
    case
        when data and not schema then opts := '-a';
        when not data and schema then opts := '-s';
        when data and schema then null;
        when not data and not schema then raise exception 'either schema or data must be included';
        end case;

    select setting from pg_config() where name = 'BINDIR' into bindir;
    create temporary table _pg_dump
    (
        id serial,
        value text
    ) on commit drop;
    ----- characters used in `copy` options:
    -----   delimiter -- ASCII Unit Separator
    -----   null -- ASCII Record Separator
    ----- The reason they are used is that they virtually never appear in standard text,
    ----- they are valid utf-8 and are single-byte characters as required by `copy`
    execute format(
            $copy$copy _pg_dump(value) from program $p$%L/pg_dump %s %L$p$ with (format text, delimiter E'\x1F', null E'\x1E') $copy$,
            bindir,
            opts,
            connstr);
    select string_agg(value, '\n' order by id) from _pg_dump into dump;
    return dump;
end;
$$;

create function dump(connstr text, schema bool default true, data bool default true) returns text
    language plpgsql
as
$$
begin
    perform omni_schema._validate_connstr(connstr);
    return omni_schema._dump(connstr, schema, data);
end;
$$;
