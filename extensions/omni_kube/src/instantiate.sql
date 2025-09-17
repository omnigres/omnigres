create function instantiate(schema regnamespace, role name default 'omni_kube') returns void
    language plpgsql
as
$instantiate$
declare
    old_search_path text := current_setting('search_path');
begin
    -- Set the search path to target schema and public
    perform set_config('search_path', schema::text || ',public', true);

    /*{% include "group_path.sql" %}*/
    /*{% include "resources_path.sql" %}*/
    /*{% include "path_with.sql" %}*/
    /*{% include "api.sql" %}*/
    execute format(
            'alter function api(text[], text, text, omni_httpc.client_certificate, text, omni_http.http_method[], jsonb[], boolean) set search_path = %L, public',
            schema::text);
    execute format(
            'alter function api(text, text, text, omni_httpc.client_certificate, text, omni_http.http_method, jsonb, boolean) set search_path = %L, public',
            schema::text);
    /*{% include "pod_credentials.sql" %}*/
    /*{% include "load_kubeconfig.sql" %}*/

    -- Core views

    create view api_group as
    select 'core' as name,
           'v1'   as preferred_version
    union all
    select data ->> 'name'                          as name,
           data -> 'preferredVersion' ->> 'version' as preferred_version
    from (select jsonb_array_elements((api('/apis')) -> 'groups') as data) groups;

    create view api_group_version as
    select 'core' as name,
           'v1'   as version,
           'v1'   as group_version
    union all
    select data ->> 'name'            as name,
           version ->> 'version'      as version,
           version ->> 'groupVersion' as group_version
    from (select jsonb_array_elements((api('/apis')) -> 'groups') as data) groups
             inner join lateral ( select version from jsonb_array_elements(groups.data -> 'versions') version) v
                        on true;

    create view api_group_openapi_v3_url as
    select case when key in ('api', 'api/v1') then 'core' else coalesce(split_part(key, '/', 2), key) end as name,
           case
               when key = 'api' then null
               when key = 'api/v1' then 'v1'
               when key ~ '.+/.+/.+' then split_part(key, '/', -2) || '/' || split_part(key, '/', -1)
               else null end
                                                                                                          as group_version,
           url
    from (select key, value ->> 'serverRelativeURL' as url
          from jsonb_each((api('/openapi/v3')) -> 'paths') paths
          where key not in ('apis', 'logs', 'version', '.well-known/openid-configuration')) apis;

    -- Resources
    /*{% include "group_resources.sql" %}*/
    execute format('alter function group_resources set search_path = %s', schema::text || ',public');
    /*{% include "resources.sql" %}*/
    execute format('alter function resources set search_path = %s', schema::text || ',public');
    /*{% include "resources_metadata.sql" %}*/
    execute format('alter function resources_metadata set search_path = %s', schema::text || ',public');
    /*{% include "resource_view.sql" %}*/
    execute format('alter function resource_view set omni_kube.search_path = %L', schema::text);
    /*{% include "resource_table.sql" %}*/
    execute format('alter function resource_table set omni_kube.search_path = %L', schema::text);
    /*{% include "watch_path.sql" %}*/
    /*{% include "watch.sql" %}*/

    /*{% include "resource_views.sql" %}*/
    execute format('alter function resource_views set omni_kube.search_path = %L', schema::text);
    /*{% include "resource_tables.sql" %}*/
    execute format('alter function resource_tables set omni_kube.search_path = %L', schema::text);

    perform from pg_roles where rolname = role;
    if not found then
        execute format('create role %I', role);
    end if;
    execute format('grant all on schema %I, omni_http, omni_httpc to %I', schema, role);
    execute format('grant select on all tables in schema %I to %I', schema, role);
    execute format('grant select, update, usage on all sequences in schema %I to %I', schema, role);
    execute format('grant execute on all functions in schema %I to %I', schema, role);

    -- Restore the path
    perform
        set_config('search_path', old_search_path, true);
end


$instantiate$;
