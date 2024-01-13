create function json_to_requirement(requirement json) returns requirement
    language sql
    immutable as
$$
select
    row (key, value)::omni_manifest.requirement
from
    json_each_text(requirement)
limit 1
$$;