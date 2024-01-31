create function jsonb_to_requirements(requirements jsonb) returns requirement[]
    language sql
    immutable as
$$
select
    array_agg(row (key, value)::omni_manifest.requirement)
from
    jsonb_each_text(requirements)
$$;