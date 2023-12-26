create table table_fs_filesystems
(
    id   integer primary key generated always as identity,
    name text not null unique
);

create table table_fs_files
(
    id            bigint primary key generated always as identity,
    filesystem_id integer              not null references table_fs_filesystems (id),
    filepath      text                 not null check (filepath != '/'),
    created_at    timestamp,
    accessed_at   timestamp,
    modified_at   timestamp,
    -- disallow 'dir' entries because they are implicit
    kind          omni_vfs_types_v1.file_kind not null check (kind != 'dir'),
    unique (filesystem_id, filepath)
);

create or replace function table_fs_files_trigger() returns trigger as
$$
declare
    result_name text;
begin
    new.filepath := omni_vfs.canonicalize_path(new.filepath, absolute => true);
    select
        filepath
    from
        omni_vfs.table_fs_files
    where
        filesystem_id = new.filesystem_id and
        (
            -- file can't be inside a file
            new.filepath like (filepath || '/%')
            or
            -- dir can't be a file
            starts_with(filepath, new.filepath || '/')
        ) and
        new.id != id
    limit 1
    into result_name;
    if found then
        if starts_with(new.filepath, result_name || '/') then
            raise exception 'can''t create file at % conflicts with an existing file %', new.filepath, result_name;
        else
            raise exception 'can''t create file at % conflicts with dir at an existing path %', new.filepath, result_name;
        end if;
    end if;
    return new;
end;
$$ language plpgsql;

create trigger table_fs_files_trigger
    before insert or
    -- don't fire trigger on update of timestamp columns
    update of id, filesystem_id, filepath, kind
    on table_fs_files
    for each row
execute function table_fs_files_trigger();


-- TODO: refactor this into [sparse] [reusable] chunks
create table table_fs_file_data
(
    file bigint primary key references table_fs_files (id),
    data bytea not null
);

/*  
    external storage optimizes fetching parts of data
    at the expense of increased storage due to disabled compression
*/
alter table omni_vfs.table_fs_file_data alter column data set storage external;

create or replace function table_fs_file_data_trigger() returns trigger as
$$
begin
    update omni_vfs.table_fs_files
    set
        created_at = (case tg_op when 'INSERT' then statement_timestamp() else created_at end),
        accessed_at = statement_timestamp(),
        modified_at = statement_timestamp()
    where
        id = new.file;
    return new;
end;
$$ language plpgsql;

create trigger table_fs_file_data_trigger
    before insert or update
    on table_fs_file_data
    for each row
execute function table_fs_file_data_trigger();

create type table_fs as
(
    id integer
);

create function table_fs(filesystem_name text) returns omni_vfs.table_fs
    language plpgsql
as
$$
declare
    result_id integer;
begin
    select
        id
    from
        omni_vfs.table_fs_filesystems
    where
        table_fs_filesystems.name = filesystem_name
    into result_id;
    if not found then
        insert
        into
            omni_vfs.table_fs_filesystems (name)
        values (filesystem_name)
        returning id into result_id;
    end if;
    return row (result_id);
end
$$;

create function list(fs table_fs, path text, fail_unpermitted boolean default true) returns setof omni_vfs_types_v1.file
    stable
    language sql
as
$$
with
    search(path) as (
        select
            case
                when omni_vfs.canonicalize_path(path, absolute => true) = '/' then ''
                else omni_vfs.canonicalize_path(path, absolute => true)
            end
    )
select
    distinct case
        -- filepath exactly matches search.path
        when filepath = search.path then row(omni_vfs.basename(filepath), kind)::omni_vfs_types_v1.file
        -- search.path is prefix of filepath
        else (
                case
                    -- only one '/' left, eg. /file
                    when split_part(substr(filepath, length(search.path)+1), '/', 3) = '' then
                        row(split_part(substr(filepath, length(search.path)+1), '/', 2), kind)::omni_vfs_types_v1.file
                    -- more than one '/', eg. /dir/file
                    else row(split_part(substr(filepath, length(search.path)+1), '/', 2), 'dir')::omni_vfs_types_v1.file
                end
        )
    end
from omni_vfs.table_fs_files, search
where filesystem_id = fs.id and
(starts_with(filepath, search.path || '/') or
filepath = search.path)
$$;

create function file_info(fs table_fs, path text) returns omni_vfs_types_v1.file_info
    stable
    language sql
as
$$
with
    search(path) as (select omni_vfs.canonicalize_path(path, absolute => true))
    select
        coalesce(length(data), 0) as size,
        created_at,
        accessed_at,
        modified_at,
        kind
    from omni_vfs.table_fs_files cross join search
    left join omni_vfs.table_fs_file_data on id = file
    where filesystem_id = fs.id and filepath = search.path
$$;


create function read(fs table_fs, path text, file_offset bigint default 0,
                     chunk_size int default null) returns bytea
as
$$
declare
    result bytea;
begin
    update
        omni_vfs.table_fs_files
    set
        accessed_at = statement_timestamp()
    where
        filesystem_id = fs.id and filepath = omni_vfs.canonicalize_path(path, absolute => true);
    
    with
        search(path) as (select omni_vfs.canonicalize_path(path, absolute => true))
        select
            case
                when chunk_size is null then substr(data, file_offset::int)
                else substr(data, file_offset::int, chunk_size)
            end
        from omni_vfs.table_fs_files cross join search
        inner join omni_vfs.table_fs_file_data on id = file
        where filesystem_id = fs.id and filepath = search.path
    into result;

    return result;
end;
$$ language plpgsql;

-- Checks

do
$$
    begin
        if not omni_vfs_types_v1.is_valid_fs('table_fs') then
            raise exception 'table_fs is not a valid vfs';
        end if;
    end;
$$ language plpgsql;