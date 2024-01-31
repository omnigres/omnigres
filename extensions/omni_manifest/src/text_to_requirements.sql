create function text_to_requirements(requirements text) returns requirement[]
    language sql
    immutable as
$$
select
    array_agg(omni_manifest.text_to_requirement(requirement))
from
    regexp_split_to_table(requirements, '\s*,\s*') r(requirement)
$$;