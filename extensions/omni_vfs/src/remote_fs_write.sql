create function write(fs remote_fs, path text, content bytea, create_file boolean default false,
                      append boolean default false) returns bigint
    language sql as
$$
select *
from
    dblink(fs.connstr, format('select omni_vfs.write((%1$s), %2$L, %3$L, %4$L, %5$L)', fs.constructor,
                              path, content, create_file, append)) t(r bigint)
$$;