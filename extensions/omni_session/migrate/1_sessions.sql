create unlogged table sessions
(
    uuid uuid primary key not null,
    touched_at timestamp default clock_timestamp()
);

/*{% include "../src/session_handler.sql" %}*/