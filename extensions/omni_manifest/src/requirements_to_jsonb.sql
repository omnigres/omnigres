create function requirements_to_jsonb(requirements requirement[]) returns jsonb
    language sql
    immutable as
$$
select
    jsonb_object_agg(name, version)
from
    unnest(requirements) r(name, version)
$$;