select identity_type('session_id', type => 'uuid', nextval => 'uuidv7');

create unlogged table sessions
(
    id session_id primary key not null default omni_session.session_id_nextval(),
    touched_at timestamp default clock_timestamp()
);

/*{% include "../src/session_handler.sql" %}*/