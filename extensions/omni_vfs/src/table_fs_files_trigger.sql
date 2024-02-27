create function table_fs_files_trigger() returns trigger as
$$
declare
    prev_component_id bigint = null;
    component         text;
    components        text[];
    components_length int;
    current           record;
    iteration_count   int    = 1;
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
                    with
                        dup_entry as (select
                                          id,
                                          kind
                                      from
                                          table_fs_files
                                      where
                                          filesystem_id = new.filesystem_id and
                                          (coalesce(parent_id, 0) = prev_component_id or prev_component_id is null) and
                                          filename = component),
                        new_entry(filesystem_id, filename, kind, parent_id, depth) as (values
                                                                                           (new.filesystem_id,
                                                                                            component,
                                                                                            'dir'::omni_vfs_types_v1.file_kind,
                                                                                            prev_component_id,
                                                                                            iteration_count)),
                        ins as (
                            insert into table_fs_files (filesystem_id, filename, kind, parent_id, depth)
                                select *
                                from
                                    new_entry
                                where
                                    not exists (select 1
                                                from dup_entry)
                                returning id, kind)
                    select
                        id,
                        kind
                    into current
                    from
                        dup_entry
                    union
                    select
                        id,
                        kind
                    from
                        ins;

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
                raise info 'hello world';
            end loop;
    end if;
    return new;
end;
$$ language plpgsql set search_path to omni_vfs;
