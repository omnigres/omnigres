create function instantiate(schema regnamespace default 'omni_shmem') returns void
    language plpgsql
as
$instantiate$
declare
    old_search_path text := current_setting('search_path');
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    --- Do we have the roles established?
    perform from pg_roles where rolname = 'omni_shmem_user';
    if not found then
        create role omni_shmem_user;
    end if;

    create function shmem_arena(name text, size bigint, transient bool default true)
        returns text
        language 'c' as
    'MODULE_PATHNAME';

    revoke execute on function shmem_arena from public;
    grant execute on function shmem_arena to omni_shmem_user;

    create function shmem_delete_arena(name text)
        returns void
        language 'c' as
    'MODULE_PATHNAME';

    revoke execute on function shmem_delete_arena from public;
    grant execute on function shmem_delete_arena to omni_shmem_user;

    create function shmem_arena_free_memory(name text)
        returns bigint
        language 'c' as
    'MODULE_PATHNAME';

    revoke execute on function shmem_arena_free_memory from public;
    grant execute on function shmem_arena_free_memory to omni_shmem_user;

    create type find_or_construct_report as
    (
        constructed boolean,
        size        bigint
    );

    create function shmem_find_or_construct(arena_name text, name text, size bigint)
        returns find_or_construct_report
        strict
        language 'c' as
    'MODULE_PATHNAME';

    execute format('alter function shmem_find_or_construct(text,text,bigint) set omni_shmem.schema = %I', schema);

    revoke execute on function shmem_find_or_construct(text, text, bigint) from public;
    grant execute on function shmem_find_or_construct(text, text, bigint) to omni_shmem_user;

    create function shmem_find_or_construct(arena_name text, name text, data bytea)
        returns find_or_construct_report
        strict
        language 'c' as
    'MODULE_PATHNAME',
    'shmem_find_or_construct_by_value';

    execute format('alter function shmem_find_or_construct(text,text,bytea) set omni_shmem.schema = %I', schema);

    revoke execute on function shmem_find_or_construct(text, text, bytea) from public;
    grant execute on function shmem_find_or_construct(text, text, bytea) to omni_shmem_user;

    create function shmem_read(arena_name text, name text, start integer default null, size integer default null)
        returns bytea
        language 'c' as
    'MODULE_PATHNAME';

    revoke execute on function shmem_read from public;
    grant execute on function shmem_read to omni_shmem_user;

    create function shmem_destroy(arena_name text, name text)
        returns boolean
        language 'c' as
    'MODULE_PATHNAME';

    revoke execute on function shmem_destroy from public;
    grant execute on function shmem_destroy to omni_shmem_user;

    if current_setting('omni_shmem.create_tests', true) != '' then
        create function run_tests()
            returns table
                    (
                        name              text,
                        passed            bool,
                        seconds           float8,
                        failure_reason    text,
                        failed_assertions text,
                        output            text
                    )
            language 'c'
        as
        'MODULE_PATHNAME';
        revoke all on function run_tests() from public;
    end if;

    create function shmem_arenas()
        returns table
                (
                    name text,
                    size bigint
                )
        language 'c'
    as
    'MODULE_PATHNAME';
    revoke execute on function shmem_arenas from public;
    grant execute on function shmem_arenas to omni_shmem_user;

    create view shmem_arenas as
    select * from shmem_arenas();

    -- Restore the path
    perform set_config('search_path', old_search_path, true);
end
$instantiate$;
