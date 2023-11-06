create type unit;

create function unit_in(cstring) returns unit
as
'MODULE_PATHNAME' language c immutable;

create function unit_out(unit) returns cstring
as
'MODULE_PATHNAME' language c immutable;

create function unit_recv(internal) returns unit
as
'MODULE_PATHNAME' language c immutable;

create function unit_send(unit) returns bytea
as
'MODULE_PATHNAME' language c immutable;


create type unit
(
    input = unit_in,
    output = unit_out,
    send = unit_send,
    receive = unit_recv,
    internallength = 1,
    alignment = char,
    storage = plain,
    passedbyvalue
);

create function unit() returns unit
as
'MODULE_PATHNAME' language c immutable;