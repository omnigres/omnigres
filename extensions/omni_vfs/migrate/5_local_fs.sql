create function write(fs local_fs, path text, content bytea, create_file boolean default false,
                      append boolean default false) returns bigint
    language c as
'MODULE_PATHNAME',
'local_fs_write';
