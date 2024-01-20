create function artifacts_to_json(artifacts artifact[]) returns json
    language sql
    immutable as
$$
select
    json_agg(a::json)
from
    unnest(artifacts) a
$$;