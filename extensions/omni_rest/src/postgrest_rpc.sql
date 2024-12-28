create or replace function _postgrest_function_call_arguments (fn regproc, passed_arguments text[])
    returns text immutable
    language sql
    as $$
    select
        string_agg(format('%1$I => $%2$s :: %3$I', name, idx, t.typname), ', ' order by idx)
    from
        pg_proc p,
        unnest(proargnames, proargtypes, proargmodes)
    with ordinality as _ (name, type, mode, idx)
    join pg_type t on t.oid = type
where
    type is not null
    and p.oid = fn
    and name::text = any (passed_arguments)
$$;

create or replace function _postgrest_function_ordered_argument_values (fn regproc, passed_arguments text[], passed_values jsonb)
    returns jsonb immutable
    language sql
    as $$
    select
        jsonb_agg(passed_values ->> (name::text)
        order by idx)
    from
        pg_proc p,
        unnest(proargnames, proargtypes, proargmodes)
    with ordinality as _ (name, type, mode, idx)
    join pg_type t on t.oid = type
where
    type is not null
    and p.oid = fn
    and name::text = any (passed_arguments)
$$;

create procedure postgrest_rpc (request omni_httpd.http_request, outcome inout omni_httpd.http_outcome, settings postgrest_settings default postgrest_settings ())
language plpgsql
as $$
declare
    function_reference regproc;
    query text;
    namespace text;
    result jsonb;
    arguments_definition text;
    argument_values jsonb;
    passed_arguments text[];
begin
    if outcome is distinct from null then
        return;
    end if;
    if request.method in ('GET', 'POST') then
        if split_part(request.path, '/', 2) <> 'rpc' then
            return;
        end if;
        call omni_rest._postgrest_function (request, function_reference, namespace, settings);
        if function_reference is null then
            return;
            -- terminate
        end if;
    else
        return;
        -- terminate;
    end if;
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
        end into passed_arguments;
    arguments_definition := coalesce(omni_rest._postgrest_function_call_arguments (function_reference, passed_arguments), '');
    if (select proretset from pg_proc where oid = function_reference) then
        query := format('select jsonb_agg(row_to_json(r.*)::jsonb) as result from %1$s(%2$s) r', function_reference, arguments_definition);
    else
        query := format('select %1$s(%2$s) as result', function_reference, arguments_definition);
    end if;
    if request.method = 'GET' then
        query := format('set transaction read only; %1$s', query);
        select
            (
                select
                    jsonb_object_agg(keys.key, values.value)
                from
                    unnest(omni_web.parse_query_string (request.query_string))
                    with ordinality as keys (key, ikey)
                    join unnest(omni_web.parse_query_string (request.query_string))
                    with ordinality as
                values (value,
                    ivalue) on ikey % 2 = 1
                    and ivalue % 2 = 0
                    and ikey = ivalue - 1) into argument_values;
    else
        select
            (
                select
                    convert_from(request.body, 'utf-8')::jsonb)
    end into argument_values;
end if;
        -- Run it
        declare message text;
        detail text;
        hint text;
        begin
            select
                omni_sql.execute (query, coalesce(omni_rest._postgrest_function_ordered_argument_values (function_reference, passed_arguments, argument_values), '[]'::jsonb)) -> 'result' into result;
            if jsonb_typeof(result) = 'array' then
              outcome := omni_httpd.http_response (result, headers => array[omni_http.http_header ('Content-Range', '0-' || jsonb_array_length(result) || '/*')]);
            else
              outcome := omni_httpd.http_response (result);
            end if;
        exception
            when others then
                get stacked diagnostics message = message_text,
                detail = pg_exception_detail,
                hint = pg_exception_hint;
            outcome := omni_httpd.http_response (jsonb_build_object('message', message, 'detail', detail, 'hint', hint), status => 400);
        end;
end;
$$;

