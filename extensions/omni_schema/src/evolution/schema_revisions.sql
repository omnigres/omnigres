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
        return query with
                         recursive
                         revisions as (select
                                           omni_vfs.dirname(name)::uuid::revision_id  as revision,
                                           omni_yaml.to_json(convert_from(
                                                   omni_vfs.read(fs, revisions_path || '/' || name),
                                                   'utf-8'))::jsonb                   as metadata,
                                           array((select
                                                      jsonb_array_elements_text(coalesce(
                                                              (omni_yaml.to_json(convert_from(
                                                                      omni_vfs.read(fs, revisions_path || '/' || name),
                                                                      'utf-8'))::jsonb) ->
                                                              'parents',
                                                              '[]'))))::revision_id[] as parents
                                       from
                                           omni_vfs.list_recursively(fs, revisions_path)
                                       where
                                           omni_vfs.basename(name) = 'metadata.yaml'),
                         all_ancestors as (select
                                               r.revision,
                                               unnest(r.parents) as ancestor
                                           from
                                               revisions r
                                           union all
                                           select
                                               aa.revision,
                                               unnest(r.parents) as ancestor
                                           from
                                               all_ancestors  aa
                                               join revisions r
                                                    on aa.ancestor = r.revision),
                         descendant_counts as (select
                                                   ancestor                    as parent,
                                                   count(distinct aa.revision) as descendant_count
                                               from
                                                   all_ancestors aa
                                               group by ancestor)
                     select
                         r.revision as revision,
                         r.parents  as parents,
                         r.metadata as metadata
                     from
                         revisions                   r
                         left join descendant_counts dc
                                   on dc.parent = r.revision
                     order by coalesce(dc.descendant_count, 0) desc;
    end if;
end;
$$;
