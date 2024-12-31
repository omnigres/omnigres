create procedure _postgrest_relation(request omni_httpd.http_request,
                                     relation inout regclass,
                                     namespace inout text,
                                     settings postgrest_settings default postgrest_settings())
    language plpgsql
as
$$
declare
    saved_search_path text := current_setting('search_path');
begin

    if request.method = 'GET' then
        namespace := omni_http.http_header_get(request.headers, 'accept-profile');
    end if;

    if request.method in ('POST', 'PATCH') then
        namespace := omni_http.http_header_get(request.headers, 'content-profile');
    end if;

    if namespace is null and cardinality(settings.schemas) > 0 then
        namespace := settings.schemas[1]::text;
    end if;

    if namespace is null then
        return;
    end if;

    if not namespace::name = any (settings.schemas) then
        relation := null;
        namespace := null;
        return;
    end if;

    perform set_config('search_path', namespace, true);

    relation := to_regclass(split_part(request.path, '/', 2));
    if relation is null or relation::text like 'pg_%' then
        relation := null;
        return;
    end if;

    perform set_config('search_path', saved_search_path, false);

end;
$$;
