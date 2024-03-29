create function modules()
    returns table
            (
                id   int8,
                path          cstring,
                omni_version  int2,
                omni_revision int2
            )
    volatile
    language c
as
'MODULE_PATHNAME';

create view modules as
    (
    select
        id,
        path::text,
        omni_version || chr(65 + omni_revision) as omni_interface
    from
        modules());

create function hooks()
    returns table
            (
                hook cstring,
                name cstring,
                module_id int8,
                pos  int
            )
    volatile
    language c
as
'MODULE_PATHNAME';

create view hooks as
    (
    select
        hook::text,
        name::text,
        module_id,
        pos
    from
        hooks());

create function shmem_allocations()
    returns table
            (
                name      cstring,
                module_id int8,
                size      int8
            )
    volatile
    language c
as
'MODULE_PATHNAME';

create view shmem_allocations as
    (
    select
        name::text,
        module_id,
        size
    from
        shmem_allocations());