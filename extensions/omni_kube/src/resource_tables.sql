create function resource_tables(base_schema name, group_versions text[] default null,
                                singular bool default true,
                                underscore_case bool default true,
                                transactional bool default false,
                                schema_template text default '%s/%s')
    returns void
    language plpgsql
as
$resource_view$
declare
    ns              name := current_setting('omni_kube.search_path');
    old_search_path text := current_setting('search_path');
    union_query text;
begin
    perform set_config('search_path', format('%I, public', ns), true);

    perform
        resource_table(case
                           when underscore_case then _camel_to_snake_case(preferred_name)
                           else preferred_name end,
                       g.name || '/' || g.preferred_version, r.name,
                       schema => case
                                     when g.name = 'core' then base_schema
                                     else format(schema_template, base_schema, g.name) end,
                       transactional => transactional)
    from
        omni_kube.api_group                                                                  g
        inner join lateral (select *,
                                   (case
                                        when singular and underscore_case then kind
                                        when singular then singular_name
                                        when not singular and underscore_case then _infer_plural_snake_case(name, kind)
                                        else name end)
                                       as preferred_name
                            from
                                omni_kube.group_resources(name || '/' || preferred_version)) r on true
    where
        (group_versions is null or (g.name || '/' || g.preferred_version = any (group_versions))) and
        singular_name != '' and
        'list' = any (verbs);

    --- Prepare a refresh
    with
        resource_functions as (select
                                   format('%I.%I()',
                                          case
                                              when g.name = 'core' then base_schema
                                              else format(schema_template, base_schema, g.name) end,
                                          format('%I',
                                                 case
                                                     when underscore_case
                                                         then _camel_to_snake_case(preferred_name)
                                                     else preferred_name end ||
                                                 '_resource_path'))
                                                                                                       as path_function,
                                   format('%I.%I',
                                          case
                                              when g.name = 'core' then base_schema
                                              else format(schema_template, base_schema, g.name) end,
                                          format('%I',
                                                 'refresh_' || case
                                                                   when underscore_case
                                                                       then _camel_to_snake_case(preferred_name)
                                                                   else preferred_name end || '_agg')) as
                                                                                                          refresh_function,
                                   row_number() over ()                                                as ordinality
                               from
                                   omni_kube.api_group                                                                  g
                                   inner join lateral (select *,
                                                              (case
                                                                   when singular and underscore_case then kind
                                                                   when singular then singular_name
                                                                   when not singular and underscore_case
                                                                       then _infer_plural_snake_case(name, kind)
                                                                   else name end)
                                                                  as preferred_name
                                                       from
                                                           omni_kube.group_resources(name || '/' || preferred_version)) r
                                              on true
                               where
                                   (group_versions is null or
                                    (g.name || '/' || g.preferred_version = any (group_versions))) and
                                   singular_name != '' and
                                   'list' = any (verbs))
    select
        format('
with api_responses as materialized (
  select response, ordinality
  from %I.api(array[%s]) with ordinality
)
%s', ns,
               string_agg(path_function, ', '),
               string_agg(
                       format('select %s(item.value)
from api_responses
cross join lateral jsonb_array_elements(response->''items'') as item(value)
where ordinality = %s',
                              refresh_function,
                              ordinality
                       ),
                       E'\n\nunion all\n\n'
               )
        )
    into union_query
    from
        resource_functions;

    perform set_config('search_path', old_search_path, true);

    execute format($schema_refresh$
    create function %1$I() returns table(events jsonb)
    begin atomic; with updates as (%2$s) select * from updates; end;
    $schema_refresh$, 'refresh_' || base_schema, union_query);

end;
$resource_view$;
