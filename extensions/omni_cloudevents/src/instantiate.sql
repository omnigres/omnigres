create function instantiate(schema regnamespace default 'omni_cloudevents') returns void
    language plpgsql
as
$$
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    -- Domain types for verifying correctness of some event fields
    create domain cloudevent_uri as text check (value is null or omni_web.text_to_uri(value) is distinct from null);
    execute format('alter domain cloudevent_uri set schema %s', schema);

    create domain cloudevent_uri_ref as cloudevent_uri check (value is null or
                                                              (omni_web.text_to_uri(value)).path is not null or
                                                              (omni_web.text_to_uri(value)).query is not null);
    execute format('alter domain cloudevent_uri_ref set schema %s', schema);


    -- The crux of it, the event
    create type cloudevent as
    (
        id              text,
        source          cloudevent_uri_ref,
        specversion     text,
        type            text,
        datacontenttype text,
        datschema       cloudevent_uri,
        subject         text,
        "time"          timestamptz,
        data            bytea,
        datatype        regtype
    );
    execute format('alter type cloudevent set schema %s', schema);

    -- Make events convertible to JSON
    create or replace function to_json(event cloudevent)
        returns json
        immutable
        language plpgsql as
    $cloudevent_to_json$
    begin
        return json_strip_nulls(json_build_object(
                'id', event.id,
                'source', event.source,
                'specversion', event.specversion,
                'type', event.type,
                'datacontenttype', event.datacontenttype,
                'datschema', event.datschema,
                'subject', event.subject,
                'time', event."time",
                'data', case
                            when event.datatype = 'json'::regtype or event.datatype = 'jsonb'::regtype
                                then convert_from(event.data, 'utf8')::json
                            when event.datatype = 'text'::regtype
                                then to_json(convert_from(event.data, 'utf8'))
                            else
                                to_json(encode(event.data, 'base64'))
                    end
                                ));
    end;
    $cloudevent_to_json$;

    create or replace function to_jsonb(event cloudevent)
        returns jsonb
        immutable
        language plpgsql as
    $cloudevent_to_jsonb$
    begin
        return to_json(event)::jsonb;
    end;
    $cloudevent_to_jsonb$;
    execute format(
            'alter function to_jsonb(cloudevent) set search_path to %I,public',
            schema);

    -- Creation of event values (does not publish them)
    create function cloudevent(id text, source cloudevent_uri_ref,
                               type text, datacontenttype text default null, datschema cloudevent_uri default null,
                               subject text default null,
                               ts timestamptz default statement_timestamp(),
                               data anyelement default null::bytea,
                               specversion text default '1.0') returns cloudevent
        language sql as
    $sql$
    select
        row (id, source, specversion, type, datacontenttype, datschema, subject, ts,
            convert_to(data::text, 'utf8'),
            pg_typeof(data))::cloudevent
    $sql$;
    execute format(
            'alter function cloudevent(text, cloudevent_uri_ref, text, text, cloudevent_uri, text, timestamptz, anyelement, text) set search_path to %I,public',
            schema);

    create function cloudevent(id uuid, source cloudevent_uri_ref,
                               type text, datacontenttype text default null, datschema cloudevent_uri default null,
                               subject text default null,
                               ts timestamptz default statement_timestamp(),
                               data anyelement default null::bytea,
                               specversion text default '1.0') returns cloudevent
        language sql as
    $sql$
    select
        cloudevent(id => id::text, source => source, type => type, datacontenttype => datacontenttype,
                   datschema => datschema,
                   subject => subject, ts => ts, data => data, specversion => specversion)
    $sql$;
    execute format(
            'alter function cloudevent(uuid, cloudevent_uri_ref, text, text, cloudevent_uri, text, timestamptz, anyelement, text) set search_path to %I,public',
            schema);

    -- We record all outgoing (published) events
    -- One of the reasons for doing this is to ensure uniqueness of (id, source)
    create table cloudevents_egress
    (
        like cloudevent,
        primary key (id, source),
        constraint source_required check (source is not null),
        constraint specversion_required check (specversion is not null),
        constraint type_required check (type is not null)
    );
    execute format('alter table cloudevents_egress set schema %s', schema);

    create or replace function cloudevents_egress_to_cloudevent(event cloudevents_egress)
        returns cloudevent as
    $cloudevents_egress_to_cloudevent$
    begin
        return row (
            event.id,
            event.source,
            event.specversion,
            event.type,
            event.datacontenttype,
            event.datschema,
            event.subject,
            event."time",
            event.data,
            event.datatype
            )::cloudevent;
    end;
    $cloudevents_egress_to_cloudevent$ language plpgsql immutable;
    execute format('alter function cloudevents_egress_to_cloudevent set schema %s', schema);

    create cast (cloudevents_egress as cloudevent)
        with function cloudevents_egress_to_cloudevent(cloudevents_egress)
        as implicit;

    -- Publish an event
    create function publish(e cloudevent) returns void
        language plpgsql as
    $publish$
    begin
        insert
        into
            cloudevents_egress
        select
            e.*;
        return;
    end;
    $publish$;
    execute format('alter function publish set search_path to %I,public', schema);

    --- NOTICE publisher
    create function notice_publisher() returns trigger
        language plpgsql
    as
    $notice_publisher$
    begin
        if tg_argv[0] = 'json' then
            raise notice '%', to_json(new::cloudevent);
        end if;
        return new;
    end;
    $notice_publisher$;
    execute format('alter function notice_publisher set search_path to %I,public', schema);

    create trigger notice_publisher
        after insert
        on cloudevents_egress
        for each row
    execute function notice_publisher(json);
    alter table cloudevents_egress
        disable trigger notice_publisher;

    -- Create NOTICE publisher
    create function create_notice_publisher() returns name
        language plpgsql as
    $create_notice_publisher$
    begin
        alter table cloudevents_egress
            enable trigger notice_publisher;
        return 'notice_publisher'::name;
    end;
    $create_notice_publisher$;
    execute format('alter function create_notice_publisher set search_path to %I,public', schema);

    -- Delete any named publisher
    create function delete_publisher(publisher name) returns void
        language plpgsql as
    $delete_publisher$
    begin
        if publisher = 'notice_publisher' then
            alter table cloudevents_egress
                disable trigger notice_publisher;
        end if;
        return;
    end;
    $delete_publisher$;
    execute format('alter function delete_publisher set search_path to %I,public', schema);

end;
$$;