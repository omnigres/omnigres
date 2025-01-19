create function instantiate(schema regnamespace default 'omni_sqlite') returns void
    language plpgsql
as
$$
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    create type sqlite;

    create function sqlite_in(cstring)
        returns sqlite
    as
    'MODULE_PATHNAME',
    'sqlite_in'
        language c immutable
                   strict;

    create function sqlite_out(sqlite)
        returns cstring
    as
    'MODULE_PATHNAME',
    'sqlite_out'
        language c immutable
                   strict;

    create type sqlite
    (
        input = sqlite_in,
        output = sqlite_out,
        alignment = int4,
        storage = 'extended',
        internallength = -1
    );

    create function sqlite_query(sqlite, text)
        returns setof record
    as
    'MODULE_PATHNAME',
    'sqlite_query'
        language c strict;

    create function sqlite_exec(sqlite, text)
        returns sqlite
    as
    'MODULE_PATHNAME',
    'sqlite_exec'
        language c strict;

    create function sqlite_serialize(sqlite)
        returns bytea
    as
    'MODULE_PATHNAME',
    'sqlite_serialize'
        language c strict;

    create function sqlite_deserialize(bytea)
        returns sqlite
    as
    'MODULE_PATHNAME',
    'sqlite_deserialize'
        language c strict;
end;
$$;