create or replace function _postgrest_function_by_arguments (namespace text, function_name text, function_arguments text[])
    returns regproc[]
    language sql
    stable
    as $$
    with arguments as (
        select
            oid,
            array_agg(coalesce(name, '')) filter (where idx <= (pronargs - pronargdefaults)) as required_parameters,
        array_agg(coalesce(name, '')) as parameters
    from
        pg_proc,
        unnest(proargnames, proargtypes, proargmodes)
        with ordinality as _ (name, type, mode, idx)
    where
        type is not null -- only input arguments
    group by
        oid
        -- we can only work with named arguments
    having
        count(*) - count(name) = 0
)
select
    array_agg(p.oid::regproc)
from
    pg_proc p
    left join arguments a on a.oid = p.oid
    left join pg_catalog.pg_namespace n on n.oid = p.pronamespace
where
    n.nspname = namespace
    and p.proname = function_name
    and coalesce(required_parameters, '{}') <@ function_arguments
    and (function_arguments <@ coalesce(parameters, '{}')
        or function_arguments = coalesce(parameters, '{}'))
$$;

create procedure _postgrest_function (request omni_httpd.http_request, function_reference inout regproc, namespace inout text, settings postgrest_settings default postgrest_settings ())
language plpgsql
as $$
declare
    saved_search_path text := current_setting('search_path');
    function_name text;
    function_arguments text[];
    all_references regproc[];
begin
    if request.method = 'GET' then
        namespace := omni_http.http_header_get (request.headers, 'accept-profile');
    else
        namespace := omni_http.http_header_get (request.headers, 'content-profile');
    end if;
    if namespace is null and cardinality(settings.schemas) > 0 then
        namespace := settings.schemas[1]::text;
    end if;
    if namespace is null then
        return;
    end if;
    if not namespace::name = any (settings.schemas) then
        function_reference := null;
        namespace := null;
        return;
    end if;
    perform
        set_config('search_path', namespace, true);
    function_name := split_part(request.path, '/', 3);
    select
        case request.method
        when 'GET' then
        (
            select
                array_agg(name)
            from
                unnest(omni_web.parse_query_string (request.query_string))
                with ordinality as _ (name, i)
            where
                i % 2 = 1)
        when 'POST' then
        (
            select
                array_agg(jsonb_object_keys)
            from
                jsonb_object_keys(convert_from(request.body, 'utf-8')::jsonb))
        end into function_arguments;
    all_references := omni_rest._postgrest_function_by_arguments (namespace, function_name, coalesce(function_arguments, '{}'));
    select
        all_references[1] into function_reference;
    if function_reference is null or function_reference::text like 'pg_%' then
        function_reference := null;
        return;
    end if;
    perform
        set_config('search_path', saved_search_path, false);
end;
$$;

