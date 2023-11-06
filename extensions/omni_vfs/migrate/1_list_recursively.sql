create function list_recursively(fs anyelement, path text, max bigint default null) returns setof omni_vfs_types_v1.file as
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
                               sub_file.kind)::omni_vfs_types_v1.file
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