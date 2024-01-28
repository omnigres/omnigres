create function is_backend_initialized() returns bool
    language c
    volatile as
'MODULE_PATHNAME';

create function hello() returns cstring
    language c
    volatile as
'MODULE_PATHNAME';