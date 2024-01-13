create function requirements_to_text(requirements requirement[]) returns text
    language sql
    immutable as
$$
with
    reqs as (select
                 omni_manifest.requirement_to_text(row (name, version)) as requirement
             from
                 unnest(requirements))
select
    string_agg(requirement, ',')
from
    reqs
$$;