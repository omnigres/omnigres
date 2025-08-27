create function watch(group_versions text[], resources text[], resource_versions text[] default null,
                      timeout int default 1, label_selectors text[] default null, field_selectors text[] default null)
    returns table
            (
                events jsonb[],
                status int2
            )
begin
    atomic;
    with resource_versions as (select resources_metadata(unnest(group_versions), unnest(resources)) ->>
                                      'resourceVersion' as resource_version),
         items as (select unnest(group_versions)                                        as group_version,
                          unnest(resources)                                             as resource,
                          unnest(array(select resource_version from resource_versions)) as resource_version,
                          unnest(watch.resource_versions) as requested_version,
                          unnest(label_selectors)         as label_selector,
                          unnest(field_selectors)         as field_selector),
         resp as (select api(array_agg(path_with(watch_path(resources_path(group_version, resource),
                                                          coalesce(requested_version, resource_version)),
                                                timeout_seconds => timeout,
                                                label_selector => label_selector,
                                                field_selector => field_selector)),
                             stream => true) as e
                  from items)
    select array(select jsonb_array_elements((e).response)), (e).status
    from resp;
end;

create function watch(group_version text, resource text, resource_version text default null, timeout int default 1,
                      label_selector text default null, field_selector text default null)
    returns table
            (
                events jsonb[],
                status int2
            )
begin
    atomic;
    select watch(array [group_version], array [resource], array [resource_version], timeout, array [label_selector],
                 array [field_selector]);
end;
