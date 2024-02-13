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

create function guc_int() returns int4
    language c
    volatile as
'MODULE_PATHNAME';

create function guc_bool() returns bool
    language c
    volatile as
'MODULE_PATHNAME';

create function guc_real() returns float8
    language c
    volatile as
'MODULE_PATHNAME';

create function guc_string() returns cstring
    language c
    volatile as
'MODULE_PATHNAME';

create function guc_enum() returns int
    language c
    volatile as
'MODULE_PATHNAME';

create function lock_mylock() returns void
    language c
    volatile as
'MODULE_PATHNAME';

create function unlock_mylock() returns void
    language c
    volatile as
'MODULE_PATHNAME';

create function mylock_tranche_id() returns int2
    language c
    volatile as
'MODULE_PATHNAME';

create function lwlock_identifier(int2) returns cstring
    language c
    volatile as
'MODULE_PATHNAME';

create function bad_shmalloc() returns void
    language c
    volatile as
'MODULE_PATHNAME';
