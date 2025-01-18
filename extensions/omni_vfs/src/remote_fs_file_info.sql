create function file_info(fs remote_fs, path text) returns omni_vfs_types_v1.file_info
    language sql as
$$
select *
from
    dblink(fs.connstr, format('select omni_vfs.file_info((%1$s), %2$L)', fs.constructor,
                              path)) t(f omni_vfs_types_v1.file_info)
$$;