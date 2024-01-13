create function requirements_to_json(requirements requirement[]) returns json
    language sql
    immutable as
$$
select
    json_object_agg(name, version)
from
    unnest(requirements) r(name, version)
$$;