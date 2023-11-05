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

create function list(fs local_fs, path text, fail_unpermitted boolean default true) returns setof omni_vfs_types_v1.file as
'MODULE_PATHNAME',
'local_fs_list' language c;

create function file_info(fs local_fs, path text) returns omni_vfs_types_v1.file_info as
'MODULE_PATHNAME',
'local_fs_file_info' language c;

create function read(fs local_fs, path text, file_offset bigint default 0,
                     chunk_size int default null) returns bytea as
'MODULE_PATHNAME',
'local_fs_read' language c;