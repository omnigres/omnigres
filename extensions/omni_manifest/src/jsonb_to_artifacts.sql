create function jsonb_to_artifacts(artifacts jsonb) returns artifact[]
    language sql
    immutable as
$$
with
    artifacts_ as (select value::omni_manifest.artifact from jsonb_array_elements(artifacts) t(value))
select
    array_agg(value)
from
    artifacts_
$$;