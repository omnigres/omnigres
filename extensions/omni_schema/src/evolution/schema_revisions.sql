create function schema_revisions(fs anyelement, revisions_path text, leafs_only boolean default false)
    returns table
            (
                revision revision_id,
                parents  revision_id[],
                metadata jsonb
            )
    language plpgsql
as
$$
declare
    rec record;
begin
    if leafs_only then
        return query
            with
                revisions as materialized (select * from schema_revisions(fs, revisions_path, leafs_only => false)),
                parent as materialized (select distinct unnest(revisions.parents) as parent_id from revisions)
            select
                r0.*
            from
                revisions r0
                left join parent on parent.parent_id = r0.revision
            where
                parent is not distinct from null;
    else
        for rec in select *
                   from
                       omni_vfs.list_recursively(fs, revisions_path)
                   where
                       omni_vfs.basename(name) = 'metadata.yaml'
            loop
                revision := omni_vfs.dirname(rec.name)::uuid;
                metadata := omni_yaml.to_json(convert_from(
                        omni_vfs.read(fs, revisions_path || '/' || rec.name), 'utf-8'));
                parents := array((select
                                      jsonb_array_elements_text(coalesce(
                                              metadata ->
                                              'parents',
                                              '[]'))))::revision_id[];
                return next;
            end loop;

        return;
    end if;
end;
$$;
