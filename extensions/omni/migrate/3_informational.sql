drop function modules() cascade;
create function modules()
    returns table
            (
                id            int8,
                path          cstring,
                name          cstring,
                identity      cstring,
                version       cstring,
                omni_version  int2,
                omni_revision int2
            )
    volatile
    language c
as
'MODULE_PATHNAME';

create view modules as
(
select id,
       path::text,
       name::text,
       identity::text,
       version::text,
       omni_version || chr(65 + omni_revision) as omni_interface
from
    modules());