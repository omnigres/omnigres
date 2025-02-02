create type urlpattern;

create function urlpattern_in(cstring) returns urlpattern
    immutable
    language c as
'MODULE_PATHNAME';

create function urlpattern_out(urlpattern)
    returns cstring
    immutable
    language c as
'MODULE_PATHNAME';

create type urlpattern
(
    like = text,
    input = urlpattern_in,
    output = urlpattern_out
);

create function matches(urlpattern, text, baseURL text default null)
    returns bool
    immutable
    language c as
'MODULE_PATHNAME',
'urlpattern_matches';

create function match(urlpattern, text, baseURL text default null)
    returns table
            (
                name      text,
                component text,
                value     text
            )
    immutable
    language c
as
'MODULE_PATHNAME',
'urlpattern_match';