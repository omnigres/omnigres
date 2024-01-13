create function text_to_requirement(requirement text) returns requirement
    language sql
    immutable as
$$
with
    arr as (select regexp_split_to_array(requirement, '=') val)
select
    row (arr.val[1], arr.val[2])::omni_manifest.requirement
from
    arr
$$;