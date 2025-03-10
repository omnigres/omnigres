create function file_info(fs table_fs, path text) returns omni_vfs_types_v1.file_info
    stable
    language sql set search_path to omni_vfs
as
$$

with
    match(id) as (
        select 
            case 
                when omni_vfs.canonicalize_path(path, absolute => true) = '/' then 
                    (select id from table_fs_files where filesystem_id = fs.id and filename = '' and depth = 1)
                else 
                    table_fs_file_id(fs, path)
            end
    )

select
    coalesce(length(d.data), 0) as size,
    d.created_at,
    d.accessed_at,
    d.modified_at,
    f.kind
from
    table_fs_files                f
    inner join match              m on f.id = m.id
     -- Use a left join because the root directory entry might not have associated data in table_fs_file_data.
    left join table_fs_file_data  d on m.id = d.file_id
    inner join table_fs_file_data d on m.id = d.file_id
where
    filesystem_id = fs.id
$$;
