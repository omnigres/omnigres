create function is_backend_initialized() returns bool
    language c
    volatile as
'MODULE_PATHNAME';

create function hello() returns cstring
    language c
    volatile as
'MODULE_PATHNAME';

create function get_shmem() returns cstring
    language c
    volatile as
'MODULE_PATHNAME';

create function get_shmem1() returns cstring
    language c
    volatile as
'MODULE_PATHNAME';

create function set_shmem(text) returns cstring
    language c
    volatile as
'MODULE_PATHNAME';

create function lookup_shmem(text) returns cstring
    language c
    volatile as
'MODULE_PATHNAME';

create function local_worker_pid() returns int4
    language c
    volatile as
'MODULE_PATHNAME';