create function table_fs_file_data_trigger() returns trigger as
$$
declare
    file_kind omni_vfs_types_v1.file_kind;
begin
    select
        kind
    into file_kind
    from
        table_fs_files
    where
        id = new.file_id;

    if file_kind != 'file' then
        raise exception 'only ''file'' kind can have data associated with it, ''%'' can''t', file_kind;
    end if;

    if tg_op = 'INSERT' then
        new.created_at = statement_timestamp();
    end if;
    new.accessed_at = statement_timestamp();
    new.modified_at = statement_timestamp();

    return new;
end;
$$ language plpgsql set search_path to omni_vfs;
