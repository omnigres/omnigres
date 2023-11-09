create type file_kind as enum ('dir', 'file');

create type file as
(
    name text,
    kind file_kind
);

create type file_info as
(
    size        bigint,
    created_at  timestamp,
    accessed_at timestamp,
    modified_at timestamp,
    kind        file_kind
);

create function is_valid_fs(type regtype) returns boolean
as
$$
declare
    result record;
begin
    select into result
    from
        pg_proc
        inner join pg_namespace on pg_namespace.nspname = 'omni_vfs' and pg_proc.pronamespace = pg_namespace.oid
    where
        pg_proc.proname = 'list' and
        pg_proc.proargtypes[0] = type::oid and
        pg_proc.proargtypes[1] = 'text'::regtype and
        pg_proc.proargtypes[2] = 'bool'::regtype and
        pg_proc.proretset and
        pg_proc.prorettype = 'omni_vfs_types_v1.file'::regtype;
    if not found then
        raise warning '%s does not define valid `omni_vfs.list` function', type::text;
        return false;
    end if;

    select into result
    from
        pg_proc
        inner join pg_namespace on pg_namespace.nspname = 'omni_vfs' and pg_proc.pronamespace = pg_namespace.oid
    where
        pg_proc.proname = 'file_info' and
        pg_proc.proargtypes[0] = type::oid and
        pg_proc.proargtypes[1] = 'text'::regtype and
        not pg_proc.proretset and
        pg_proc.prorettype = 'omni_vfs_types_v1.file_info'::regtype;
    if not found then
        raise warning '%s does not define valid `omni_vfs.file_info` function', type::text;
        return false;
    end if;

    select into result
    from
        pg_proc
        inner join pg_namespace on pg_namespace.nspname = 'omni_vfs' and pg_proc.pronamespace = pg_namespace.oid
    where
        pg_proc.proname = 'read' and
        pg_proc.proargtypes[0] = type::oid and
        pg_proc.proargtypes[1] = 'text'::regtype and
        pg_proc.proargtypes[2] = 'bigint'::regtype and
        pg_proc.proargtypes[3] = 'int'::regtype and
        not pg_proc.proretset and
        pg_proc.prorettype = 'bytea'::regtype;
    if not found then
        raise warning '%s does not define valid `omni_vfs.read` function', type::text;
        return false;
    end if;
    return true;
end;
$$ language plpgsql;