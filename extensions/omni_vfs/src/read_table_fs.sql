create function read(fs table_fs, path text, file_offset bigint default 0,
                     chunk_size int default null) returns bytea
as
$$
declare
    match_id bigint = table_fs_file_id(fs, path);
    result   bytea;
begin
    update
        omni_vfs.table_fs_file_data
    set
        accessed_at = statement_timestamp()
    where
        file_id = match_id
    returning
        (
            case
                when chunk_size is null then substr(data, file_offset::int)
                else substr(data, file_offset::int, chunk_size)
                end
            )
        into result;

    return result;
end;
$$ language plpgsql set search_path to omni_vfs;