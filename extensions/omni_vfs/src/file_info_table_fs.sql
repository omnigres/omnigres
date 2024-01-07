create function file_info(fs table_fs, path text) returns omni_vfs_types_v1.file_info
    stable
    language sql set search_path to omni_vfs
as
$$

with
    match(id) as (select table_fs_file_id(fs, path))
select
    coalesce(length(d.data), 0) as size,
    d.created_at,
    d.accessed_at,
    d.modified_at,
    f.kind
from
    table_fs_files                f
    inner join match              m on f.id = m.id
    inner join table_fs_file_data d on m.id = d.file_id
where
    filesystem_id = fs.id
$$;
