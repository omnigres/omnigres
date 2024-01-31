create function list(fs table_fs, path text, fail_unpermitted boolean default true) returns setof omni_vfs_types_v1.file
    stable
    language sql set search_path to omni_vfs
as
$$
with
    match(id) as (select table_fs_file_id(fs, path))
select
    f.filename,
    f.kind
from
    table_fs_files   f
    inner join match m on coalesce(f.parent_id, 0) = m.id or (f.id = m.id and f.kind != 'dir');
$$;