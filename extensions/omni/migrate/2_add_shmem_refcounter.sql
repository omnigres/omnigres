drop view omni.shmem_allocations;
drop function shmem_allocations();

create function shmem_allocations()
    returns table
            (
                name       cstring,
                module_id  int8,
                size       int8,
                refcounter int4
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
        size,
        refcounter
    from
        shmem_allocations());