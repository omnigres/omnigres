create function watch(group_versions text[], resources text[], resource_versions text[] default null,
                      timeout int default 1, label_selectors text[] default null, field_selectors text[] default null)
    returns table
            (
                events jsonb[],
                status   int2
            )
begin
    atomic;
    with resource_versions as (select resources_metadata(unnest(group_versions), unnest(resources)) ->>
                                      'resourceVersion' as resource_version),
         items as (select unnest(group_versions)                                        as group_version,
                          unnest(resources)                                             as resource,
                          unnest(array(select resource_version from resource_versions)) as resource_version,
                          unnest(watch.resource_versions)                               as requested_version),
         resp as (select api(array_agg('/' ||
                                              case
                                                  when group_version in ('v1')
                                                      then 'api/v1'
                                                  else 'apis/' || group_version end ||
                                              '/' ||
                                              resource || '?watch=1&resourceVersion=' ||
                                              coalesce(requested_version, resource_version) || '&timeoutSeconds=' ||
                                       timeout),
                             label_selectors => label_selectors, field_selectors => field_selectors,
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
