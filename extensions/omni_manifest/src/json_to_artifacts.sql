create function json_to_artifacts(artifacts json) returns artifact[]
    language sql
    immutable as
$$
with
    artifacts_ as (select value::omni_manifest.artifact from json_array_elements(artifacts) t(value))
select
    array_agg(value)
from
    artifacts_
$$;