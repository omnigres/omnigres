create function json_to_requirements(requirements json) returns requirement[]
    language sql
    immutable as
$$
select
    array_agg(row (key, value)::omni_manifest.requirement)
from
    json_each_text(requirements)
$$;