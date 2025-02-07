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
                                cookie_name text default '_session',
    http_only bool default true,
    secure bool default true,
    same_site text default 'Lax',
    domain text default null,
    max_age int default null,
    expires timestamptz default null,
    partitioned bool default false,
    path text default null
) returns omni_httpd.http_outcome
    language plpgsql as
$$
declare
    _response omni_httpd.http_response;
    _session  uuid;
begin
    if same_site not in ('Strict', 'Lax', 'None') then
        raise exception 'same_site should be Strict, Lax or None';
    end if;
    if same_site = 'None' then
        secure := true;
    end if;
    _session := omni_var.get('omni_session.session', null::omni_session.session_id);
    if _session is null then
        return response;
    end if;
    _response := response::omni_httpd.http_response;
    if _response is null then
        return response;
    end if;
    _response.headers = _response.headers || omni_http.http_header('set-cookie', cookie_name || '=' || _session ||
                                             case when http_only then '; HttpOnly' else '' end ||
                                             case when secure then '; Secure' else '' end ||
                                             case when same_site is not null then '; SameSite=' || same_site else '' end ||
                                             case when domain is not null then '; Domain=' || domain else '' end ||
                                             case when max_age is not null then '; Max-Age=' || max_age else '' end ||
                                             case when expires is not null then '; Expires=' || to_char(expires at time zone 'GMT', 'Dy, DD Mon YYYY HH24:MI:SS "GMT"') else '' end ||
                                             case when partitioned then '; Partitioned' else '' end ||
                                             case when path is not null then '; Path=' || path else '' end
                                             );
    return _response::omni_httpd.http_outcome;
end;
$$;