create function jsonb_to_requirement(requirement jsonb) returns requirement
    language sql
    immutable as
$$
select
    row (key, value)::omni_manifest.requirement
from
    jsonb_each_text(requirement)
limit 1
$$;