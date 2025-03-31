create function instantiate_resend(schema regnamespace default 'omni_email',
                                   credentials name default 'omni_credentials.credentials') returns void
    language plpgsql
as
$$
declare
    old_search_path text := current_setting('search_path');
begin
    perform from pg_extension where extname = 'omni_httpc';
    if not found then
        raise exception 'omni_httpc extension is required';
    end if;

    begin
        perform credentials::regclass;
    exception
        when others then
            raise exception 'omni_credentials table % must be available', credentials;
    end;

    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public,omni_polyfill,pg_catalog', true);

    create table email_message_sent_via_resend
    (
        id        email_message_id not null references email_message (id),
        resend_id uuid             not null unique
    );

    create function send_email_resend(id email_message_id) returns bool
        language plpgsql as
    $send_email_resend$
    declare
        api_key text;
        res     omni_httpc.http_response;
    begin
        execute format('select value from %s where name = %L and kind = %L',
                       current_setting('omni_credentials.credentials_table'), 'resend.com', 'api_secret') into api_key;
        if api_key is null then
            raise exception 'credential api:resend.com is not defined';
        end if;
        select
            (omni_httpc.http_execute(omni_httpc.http_request('https://api.resend.com/emails', method => 'POST',
                                                             headers => array [omni_http.http_header('Authorization', 'Bearer ' || api_key),
                                                                 omni_http.http_header('Content-Type', 'application/json')
                                                                 ],
                                                             body => convert_to(
                                                                     jsonb_build_object('from', sender_address, 'to',
                                                                                        recipient_addresses, 'subject',
                                                                                        subject, 'text', text_body,
                                                                                        'html', html_body)::text,
                                                                     'utf-8')))).*
        into res
        from
            email_message
            natural left join email_message_subject
            natural left join email_message_text_body
            natural left join email_message_html_body
            natural left join email_message_sender
            left join         lateral ( select
                                            jsonb_agg(emr.recipient_address) as recipient_addresses
                                        from
                                            email_message_recipient                                emr
                                            natural left join email_message_recipient_display_name emrdn
                                        where
                                            emr.id = email_message.id) raddr on true
        where
            email_message.id = send_email_resend.id;
        if res.status != 200 then
            raise exception '%', convert_from(res.body, 'utf8')::json;
        end if;
        insert
        into
            email_message_sent
        values (id);
        insert
        into
            email_message_sent_via_resend
        values (id, ((convert_from(res.body, 'utf8')::jsonb) ->> 'id')::uuid);
        return res.status = 200;
    end;
    $send_email_resend$;
    execute format('alter function send_email_resend set omni_credentials.credentials_table = %I', credentials);
    execute format('alter function send_email_resend set search_path = %s,public', schema);


    perform set_config('search_path', old_search_path, true);
end;
$$;
