create function read(fs remote_fs, path text, file_offset bigint default 0,
                     chunk_size int default null) returns bytea
    language sql as
$$
select *
from
    dblink(fs.connstr, format('select omni_vfs.read((%1$s), %2$L, %3$L, %4$L)', fs.constructor,
                              path, file_offset, chunk_size)) t(data bytea)
$$;