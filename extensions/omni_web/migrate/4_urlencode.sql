create function url_encode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;
create function url_decode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;

create function uri_encode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;
create function uri_decode(text)
    returns text
as
'MODULE_PATHNAME'
    language c
    immutable
    strict;