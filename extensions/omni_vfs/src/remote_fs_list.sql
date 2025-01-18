create function list(fs remote_fs, path text, fail_unpermitted boolean default true) returns setof omni_vfs_types_v1.file
    language sql as
$$
select *
from
    dblink(fs.connstr, format('select omni_vfs.list((%1$s), %2$L, %3$L)', fs.constructor,
                              path, fail_unpermitted)) t(f omni_vfs_types_v1.file)
$$;