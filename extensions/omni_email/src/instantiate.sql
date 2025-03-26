create function instantiate(schema regnamespace default 'omni_email') returns void
    language plpgsql
as
$$
declare
    old_search_path text := current_setting('search_path');
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public,omni_polyfill,pg_catalog', true);

    -- Core

    create domain email_address as text check ( position('@' in value) > 0 );

    perform identity_type('email_message_id', type => 'uuid', nextval => 'uuidv7()');

    create table email_message
    (
        id email_message_id primary key default email_message_id_nextval()
    );

    create table email_message_sender
    (
        id             email_message_id not null unique references email_message (id),
        sender_address email_address    not null,
        unique (id, sender_address)
    );

    create table email_message_sender_display_name
    (
        id                  email_message_id not null,
        sender_address      email_address    not null,
        sender_display_name text             not null,
        foreign key (id, sender_address) references email_message_sender (id, sender_address)
    );

    create table email_message_subject
    (
        id      email_message_id not null references email_message (id),
        subject text             not null
    );

    create type email_message_recipient_type as enum ('To','Cc','Bcc');

    create table email_message_recipient
    (
        id                email_message_id             not null references email_message (id),
        recipient_address email_address                not null,
        recipient_type    email_message_recipient_type not null default 'To',
        unique (id, recipient_address)
    );

    create table email_message_recipient_display_name
    (
        id                     email_message_id not null,
        recipient_address      email_address    not null,
        recipient_display_name text             not null,
        foreign key (id, recipient_address) references email_message_recipient (id, recipient_address)
    );

    create table email_message_text_body
    (
        id        email_message_id not null unique references email_message (id),
        text_body text             not null
    );

    create table email_message_html_body
    (
        id        email_message_id not null unique references email_message (id),
        html_body text             not null
    );

    create type email_attachment_disposition as enum ('attachment','inline');

    create table email_message_attachment
    (
        id                     email_message_id             not null references email_message (id),
        attachment_file_name   text                         not null,
        attachment_mime_type   text                         not null,
        attachment_file        bytea                        not null,
        attachment_disposition email_attachment_disposition not null default 'attachment'
    );

    create table email_message_header
    (
        id           email_message_id not null references email_message (id),
        header_name  text             not null,
        header_value text             not null,
        unique (id, header_name)
    );

    --- View
    create view email_message_view as
        select
            id,
            sender_display_name,
            sender_address,
            recipients,
            subject,
            text_body,
            html_body
        from
            email_message
            natural left join email_message_sender
            natural left join email_message_sender_display_name
            natural left join email_message_subject
            natural left join email_message_recipient
            natural left join email_message_text_body
            natural left join email_message_html_body
            natural left join email_message_attachment
            natural left join email_message_header
            left join         lateral ( select
                                                    array_agg(array [email_message_recipient.recipient_type, recipient_display_name, recipient_address]::text[])
                                                    over (partition by id) as recipients
                                        from
                                            email_message_recipient emr
                                            natural left join email_message_recipient_display_name where emr.id = email_message.id) recipients on true;

    -- Outbox
    create table email_message_outbox
    (
        id email_message_id not null unique references email_message (id)
    );
    create view email_message_outbox_available as
        select * from email_message_outbox for update skip locked;

    create table email_message_sent
    (
        id email_message_id not null unique references email_message (id)
    );

    create function email_message_sent() returns trigger
        language plpgsql as
    $email_message_sent$
    begin
        delete from email_message_outbox where id = new.id;
        return new;
    end;
    $email_message_sent$;

    execute format('alter function email_message_sent set search_path to %I,public', schema);

    create trigger email_message_sent
        after insert
        on email_message_sent
        for each row
    execute function email_message_sent();

    perform set_config('search_path', old_search_path, true);
end;
$$;
