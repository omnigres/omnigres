create function table_fs(filesystem_name text) returns omni_vfs.table_fs
    language plpgsql
    set search_path to omni_vfs
as
$$
declare
    result_id integer;
begin
    raise info 'hello world';
    select
        id
    from
        table_fs_filesystems
    where
        table_fs_filesystems.name = filesystem_name
    into result_id;
    if not found then
        insert
        into
            table_fs_filesystems (name)
        values (filesystem_name)
        returning id into result_id;
    end if;
    return row (result_id);
end
$$;