create function session_handler(session_id session_id) returns uuid
    language plpgsql as
$$
declare
    _session uuid;
    _ins     uuid;
    _upd     uuid;
begin
    with touch
             as (update omni_session.sessions set touched_at = clock_timestamp() where id = session_id returning id),
         insertion as (
             insert
                 into omni_session.sessions (id)
                     (select omni_session.session_id_nextval() where not exists(select from touch))
                     returning id)
    select omni_var.set('omni_session.session', coalesce(insertion.id, touch.id)), insertion.id, touch.id
    into _session, _ins, _upd
    from (values (null::uuid)) t (c)
             left join insertion on true
             left join touch on true;
    return _session;
end;
$$;

create function session_handler(request omni_httpd.http_request, cookie_name text default '_session') returns omni_httpd.http_request
    language plpgsql as
$$
begin
    perform omni_session.session_handler(omni_session.session_id(omni_web.cookie_get(
            omni_http.http_header_get(request.headers, 'cookie'),
            cookie_name)::uuid));
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
    _session := omni_var.get('omni_session.session', null::omni_session.session_id);
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