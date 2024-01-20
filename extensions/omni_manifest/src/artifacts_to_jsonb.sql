create function artifacts_to_jsonb(artifacts artifact[]) returns jsonb
    language sql
    immutable as
$$
select
    jsonb_agg(a::jsonb)
from
    unnest(artifacts) a
$$;