create type uri as
(
    scheme    text,
    user_info text,
    host      text,
    path      text,
    port integer,
    query     text,
    fragment  text
);

create function text_to_uri(text) returns uri
    immutable
    strict
    language c as
'MODULE_PATHNAME';

create cast (text as uri) with function text_to_uri(text);