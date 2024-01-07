create table table_fs_filesystems
(
    id   integer primary key generated always as identity,
    name text not null unique
);

create table table_fs_files
(
    id            bigint primary key generated always as identity,
    filesystem_id integer              not null references table_fs_filesystems (id),
    filename      text                 not null,
    kind          omni_vfs_types_v1.file_kind not null,
    parent_id     bigint references table_fs_files(id),
    depth         int
);

/*
    Although parent_id will be stored as null for root paths, but use coalesce(parent_id, 0)
    in where and join on clauses so that postgres planner has the option of using unique index
    as well
    
    picked 0 for coalesce because id is generated column and it starts with 1 so 0 won't be generated
*/
create unique index unique_filepath on table_fs_files (filename, coalesce(parent_id, 0), filesystem_id);

/*{% include "../src/table_fs_files_trigger.sql" %}*/

create trigger table_fs_files_trigger
    before insert or update
    on table_fs_files
    for each row
execute function table_fs_files_trigger();

create type table_fs as
(
    id integer
);

/*{% include "../src/table_fs.sql" %}*/
/*{% include "../src/table_fs_file_id.sql" %}*/

-- TODO: refactor this into [sparse] [reusable] chunks
create table table_fs_file_data
(
    file_id bigint primary key references table_fs_files (id),
    created_at    timestamp,
    accessed_at   timestamp,
    modified_at   timestamp,
    data bytea not null
);

/*  
    external storage optimizes fetching parts of data
    at the expense of increased storage due to disabled compression
*/
alter table omni_vfs.table_fs_file_data alter column data set storage external;

/*{% include "../src/table_fs_file_data_trigger.sql" %}*/

create trigger table_fs_file_data_trigger
    before insert or update of file_id, data
    on table_fs_file_data
    for each row
execute function table_fs_file_data_trigger();

/*{% include "../src/list_table_fs.sql" %}*/
/*{% include "../src/file_info_table_fs.sql" %}*/
/*{% include "../src/read_table_fs.sql" %}*/