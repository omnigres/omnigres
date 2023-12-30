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

alter table table_fs_files add constraint unique_filepath unique nulls not distinct (filename, parent_id, filesystem_id);

create function table_fs_files_trigger() returns trigger as
$$
declare
    prev_component_id bigint = null;
    component text;
    components text[];
    components_length int;
    current record;
    iteration_count int = 1;
begin
    if tg_op = 'UPDATE' then
        raise exception 'update of table_fs_files is not allowed';
    end if;
    if pg_trigger_depth() = 1 then
        new.filename = omni_vfs.canonicalize_path(new.filename, absolute => true);
        if new.filename = '/' then
            new.filename = '';
        end if;
        components = regexp_split_to_array(new.filename, '/');
        components_length = array_length(components, 1);
        foreach component in array components
        loop
            if iteration_count < components_length then
                with dup_entry as (
                    select id, kind from table_fs_files
                    where filesystem_id = new.filesystem_id and (parent_id = prev_component_id or prev_component_id is null) and filename = component
                ), new_entry(filesystem_id, filename, kind, parent_id, depth) as (
                    values
                    (new.filesystem_id, component, 'dir'::omni_vfs_types_v1.file_kind, prev_component_id, iteration_count)
                ), ins as (
                    insert into table_fs_files(filesystem_id, filename, kind, parent_id, depth)
                    select * from new_entry
                    where not exists (
                        select 1 from dup_entry
                    )
                    returning id, kind
                )
                select id, kind into current 
                from dup_entry
                union 
                select id, kind from ins;

                if current.kind != 'dir' then
                    raise exception 'conflicts with an existing % ''%''', current.kind, array_to_string(components[:iteration_count], '/');
                end if;

                prev_component_id = current.id;
            else
                new.filename = component;
                new.parent_id = prev_component_id;
                new.depth = iteration_count;
            end if;

            iteration_count = iteration_count + 1;
        end loop;
    end if;
    return new;
end;
$$ language plpgsql set search_path to omni_vfs;

create trigger table_fs_files_trigger
    before insert or update
    on table_fs_files
    for each row
execute function table_fs_files_trigger();

create type table_fs as
(
    id integer
);

create function table_fs(filesystem_name text) returns omni_vfs.table_fs
    language plpgsql set search_path to omni_vfs
as
$$
declare
    result_id integer;
begin
    select
        id
    from
        table_fs_filesystems
    where
        table_fs_filesystems.name = filesystem_name
    into result_id;
    if not found then
        insert
        into
            table_fs_filesystems (name)
        values (filesystem_name)
        returning id into result_id;
    end if;
    return row (result_id);
end
$$;

create function table_fs_file_id(fs table_fs, path text) returns bigint
    stable
    language sql set search_path to omni_vfs
as
$$
with query as (
    with components as (
        select *
        from
            regexp_split_to_table(
                case
                    when omni_vfs.canonicalize_path(path, absolute => true) = '/' then ''
                    else omni_vfs.canonicalize_path(path, absolute => true)
                end,
                '/'
            )
        with ordinality x(component, depth)
    ), depth as (
        select max(depth) as max_depth from components
    )
    select *
    from components, depth
), match as (
    with recursive r as (
        select
            f.depth, q.max_depth,
            f.id, f.filename, f.kind
        from
            table_fs_files f
        inner join query q on f.filesystem_id = fs.id and f.depth = 1 and q.depth = 1 and q.component = f.filename
        union
        select
            f.depth, q.max_depth,
            f.id, f.filename, f.kind
        from
            r
        inner join table_fs_files f on f.parent_id = r.id
        inner join query q on q.depth = r.depth + 1 and q.component = f.filename
    )
    select id from r where r.depth = r.max_depth
)
select (select * from match);
$$;


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

create function table_fs_file_data_trigger() returns trigger as
$$
declare
    file_kind omni_vfs_types_v1.file_kind;
begin
    select kind into file_kind
    from table_fs_files
    where id = new.file_id;

    if file_kind != 'file' then
        raise exception 'only ''file'' kind can have data associated with it, ''%'' can''t', file_kind;
    end if;

    if tg_op = 'INSERT' then
        new.created_at = statement_timestamp();
    end if;
    new.accessed_at = statement_timestamp();
    new.modified_at = statement_timestamp();

    return new;
end;
$$ language plpgsql set search_path to omni_vfs;

create trigger table_fs_file_data_trigger
    before insert or update of file_id, data
    on table_fs_file_data
    for each row
execute function table_fs_file_data_trigger();

create function list(fs table_fs, path text, fail_unpermitted boolean default true) returns setof omni_vfs_types_v1.file
    stable
    language sql set search_path to omni_vfs
as
$$
with match(id) as (
    select table_fs_file_id(fs, path)
)
select f.filename, f.kind
from table_fs_files f
inner join match m on f.parent_id = m.id or (f.id = m.id and f.kind != 'dir');
$$;

create function file_info(fs table_fs, path text) returns omni_vfs_types_v1.file_info
    stable
    language sql set search_path to omni_vfs
as
$$

with match(id) as (
    select table_fs_file_id(fs, path)
)
select
    coalesce(length(d.data), 0) as size,
    d.created_at,
    d.accessed_at,
    d.modified_at,
    f.kind
from table_fs_files f
inner join match m on f.id = m.id
inner join table_fs_file_data d on m.id = d.file_id
where filesystem_id = fs.id
$$;


create function read(fs table_fs, path text, file_offset bigint default 0,
                     chunk_size int default null) returns bytea
as
$$
declare
    match_id bigint = table_fs_file_id(fs, path);
    result bytea;
begin
    update
        omni_vfs.table_fs_file_data
    set
        accessed_at = statement_timestamp()
    where
        file_id = match_id
    returning
        (
            case
                when chunk_size is null then substr(data, file_offset::int)
                else substr(data, file_offset::int, chunk_size)
            end
        )
    into result;

    return result;
end;
$$ language plpgsql set search_path to omni_vfs;

-- Checks

do
$$
    begin
        if not omni_vfs_types_v1.is_valid_fs('table_fs') then
            raise exception 'table_fs is not a valid vfs';
        end if;
    end;
$$ language plpgsql;