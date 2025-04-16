create or replace function proxy_handler (request omni_httpd.http_request)
    returns omni_httpd.http_outcome
    language plpgsql
    as $function$
declare
    orb_name_from_request text := split_part(request.path, '/', 2);
    orb_path_from_request text := regexp_replace(request.path, '\/.*?\/', '');
    only_possible_target text := (
        case when (
            select
                count(*)
            from
                pg_database
            where
                datname not in (current_database(), 'omnigres', 'postgres')
                and not datistemplate) = 1 then
            (
                select
                    datname
                from
                    pg_database
                where
                    datname not in (current_database(), 'omnigres', 'postgres')
                    and not datistemplate)
        end);
    orb_name text := coalesce(only_possible_target, orb_name_from_request);
    -- it will take the orb name as prefix even when it is the only option
    -- this is useful when designing modular systems namespaced by path from scratch
    orb_path text := case when only_possible_target is not null and only_possible_target <> orb_name_from_request then
        request.path
    else
        '/' || orb_path_from_request
    end;
    orb_conn text := format('dbname=%s host=%s port=%s user=%s', orb_name, current_setting('unix_socket_directories'), current_setting('port'), current_user);
    orb_port omni_httpd.port;
begin
    -- cannot route if we dont have a target defined
    if orb_name is null then
        return omni_httpd.http_response (status => 404);
    end if;

    if not exists (
        select
        from
            pg_database
        where
            datname = orb_name
            and datname not in ('omnigres', 'postgres')
            and not datistemplate) then
    -- the orb name is nowhere to be found
    return omni_httpd.http_response (status => 404);
end if;
    declare message text;
    detail text;
    hint text;
    begin
        select
            effective_port
        from
            dblink(orb_conn, $$
                select
                    effective_port from omni_httpd.listeners l
                    where
                        l.address | '127.0.0.1' = '127.0.0.1' limit 1$$) l (effective_port omni_httpd.port) into orb_port;
    exception
        when others then
            get stacked diagnostics message = message_text,
            detail = pg_exception_detail,
            hint = pg_exception_hint;
            return omni_httpd.http_response (jsonb_build_object('message', message, 'detail', detail, 'hint', hint), status => 400);
    end;
    -- return omni_httpd.http_response(body => format('http://127.0.0.1:%s/%s', orb_port, orb_path));
    return omni_httpd.http_proxy (format('http://127.0.0.1:%s%s', orb_port, orb_path));
end;

$function$;

create or replace function proxy_listener (listener_id int)
    returns void
    language plpgsql
    as $function$
begin
    execute format(
        $def$
        create view %I as 
        select 
            omni_httpd.urlpattern ('/*?', port => (select effective_port from omni_httpd.listeners l where l.id = %s)) as match,
            '@extschema@.proxy_handler(omni_httpd.http_request)'::regprocedure as handler;
        $def$,
        'proxy_listener_' || listener_id::text || '_router',
        listener_id
    );
end;
$function$;

-- do we route using path?
-- yes or lowest subdomain
-- if there is only one we just redirect there
