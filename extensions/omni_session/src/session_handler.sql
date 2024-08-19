create function session_handler(request omni_httpd.http_request, cookie_name text default '_session') returns omni_httpd.http_request
    language plpgsql as
$$
declare
    _session uuid;
    _ins uuid;
    _upd uuid;
begin
    with cookie
             as (select omni_web.cookie_get(omni_http.http_header_get(request.headers, 'cookie'),
                                            cookie_name)::uuid as value),
         touch
             as (update omni_session.sessions set touched_at = clock_timestamp() from cookie where uuid = cookie.value returning uuid),
         insertion as (
             insert
                 into omni_session.sessions (uuid)
                         (select gen_random_uuid() where not exists(select from touch))
                     returning uuid)
    select omni_var.set('omni_session.session', coalesce(insertion.uuid, touch.uuid)), insertion.uuid, touch.uuid
    into _session, _ins, _upd
    from cookie
             left join insertion on true
             left join touch on true;
    return request;
end;
$$;

create function session_handler(response omni_httpd.http_outcome,
                                cookie_name text default '_session') returns omni_httpd.http_outcome
    language plpgsql as
$$
declare
    _response omni_httpd.http_response;
    _session  uuid;
begin
    _session := omni_var.get('omni_session.session', null::uuid);
    if _session is null then
        return response;
    end if;
    _response := response::omni_httpd.http_response;
    if _response is null then
        return response;
    end if;
    _response.headers = _response.headers || omni_http.http_header('set-cookie', cookie_name || '=' || _session);
    return _response::omni_httpd.http_outcome;
end;
$$;