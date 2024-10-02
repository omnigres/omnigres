create type regex;

create function regex_in(cstring) returns regex
    immutable
    returns null on null input
    language c as
'MODULE_PATHNAME';

create function regex_out(regex) returns cstring
    immutable
    returns null on null input
    language c as
'MODULE_PATHNAME';

create type regex
(
    internallength = -1,
    input = regex_in,
    output = regex_out,
    storage = extended
);

create function regex_named_groups(regex)
    returns table
            (
                name   cstring,
                number int
            )
    parallel safe
    immutable
    strict
    language c
as
'MODULE_PATHNAME';

create function regex_match(text, regex)
    returns text[]
    parallel safe
    immutable
    strict
    language c
as
'MODULE_PATHNAME';

create function regex_matches(text, regex)
    returns setof text[]
    immutable
    parallel safe
    strict
    language c
as
'MODULE_PATHNAME';


create function regex_text_matches(subject text, pattern regex) returns boolean
    immutable
    parallel safe
    returns null on null input
as
'MODULE_PATHNAME'
    language c;

create function regex_matches_text(pattern regex, subject text) returns boolean
    immutable
    parallel safe
    returns null on null input
as
'MODULE_PATHNAME'
    language c;

create function regex_text_matches_not(subject text, pattern regex) returns boolean
    immutable
    parallel safe
    returns null on null input
as
'MODULE_PATHNAME'
    language c;

create function regex_matches_text_not(pattern regex, subject text) returns boolean
    immutable
    parallel safe
    returns null on null input
as
'MODULE_PATHNAME'
    language c;

create operator =~ (
    procedure = regex_text_matches,
    leftarg = text,
    rightarg = regex
    );

create operator ~ (
    procedure = regex_text_matches,
    leftarg = text,
    rightarg = regex
    );

create operator ~ (
    procedure = regex_matches_text,
    leftarg = regex,
    rightarg = text,
    commutator = ~
    );

create operator !~ (
    procedure = regex_text_matches_not,
    leftarg = text,
    rightarg = regex,
    negator = ~
    );

create operator !~ (
    procedure = regex_matches_text_not,
    leftarg = regex,
    rightarg = text,
    commutator = !~,
    negator = ~
    );
