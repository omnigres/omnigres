create function write(fs table_fs, path text, content bytea, create_file boolean default false,
                      append boolean default false) returns bigint
as
$$
declare
    _file_id bigint  = table_fs_file_id(fs, path);
    created  boolean := false;
begin
    if _file_id is null then
        if create_file then
            insert into omni_vfs.table_fs_files (filesystem_id, filename, kind)
            values (fs.id, path, 'file')
            returning id into _file_id;
            created := true;
        else
            raise exception 'file not found';
        end if;
    end if;
    if created then
        insert into omni_vfs.table_fs_file_data (data, file_id)
        values (content, _file_id);
    else
        update
            omni_vfs.table_fs_file_data
        set data = case when append then data || content else content end
        where file_id = _file_id;
    end if;

    return octet_length(content);
end;
$$ language plpgsql set search_path to omni_vfs;