create function memory_segments()
    returns table
            (
                name    cstring,
                address int8,
                size int8
            )
    volatile
    language c
as
'MODULE_PATHNAME';

create view memory_segments as
(
select name::text,
       address,
       size
from
    memory_segments());