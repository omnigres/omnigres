create function remote_fs(connstr text, constructor text) returns remote_fs
    language sql
as
$$
select row (connstr, constructor)::omni_vfs.remote_fs
$$;