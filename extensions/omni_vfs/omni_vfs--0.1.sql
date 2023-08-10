--- local_fs
create table local_fs_mounts
(
    id    integer primary key generated always as identity,
    mount text not null unique
);

alter table local_fs_mounts
    enable row level security;

create type local_fs as
(
    id integer
);

create function local_fs(mount text) returns local_fs as
'MODULE_PATHNAME' language c;

create function list(fs local_fs, path text, fail_unpermitted boolean default true) returns setof omni_vfs_api.file as
'MODULE_PATHNAME',
'local_fs_list' language c;

create function file_info(fs local_fs, path text) returns omni_vfs_api.file_info as
'MODULE_PATHNAME',
'local_fs_file_info' language c;

create function read(fs local_fs, path text, file_offset bigint default 0,
                     chunk_size int default null) returns bytea as
'MODULE_PATHNAME',
'local_fs_read' language c;

-- Helpers

create function list_recursively(fs anyelement, path text, max bigint default null) returns setof omni_vfs_api.file as
$$
with
    recursive
    directory_tree as (select
                           file
                       from
                           lateral omni_vfs.list(fs, path) as file -- Top directory
                       union all
                       select
                           row ((directory_tree.file).name || '/' || sub_file.name,
                               sub_file.kind)::omni_vfs_api.file
                       from
                           directory_tree,
                           lateral omni_vfs.list(fs, (directory_tree.file).name, fail_unpermitted => false) as sub_file
                       where
                           (directory_tree.file).name != sub_file.name)
select *
from
    directory_tree
limit max;
$$
    language sql;

-- Checks

do
$$
    begin
        if not omni_vfs_api.is_valid_fs('local_fs') then
            raise exception 'local_fs is not a valid vfs';
        end if;
    end;
$$ language plpgsql
