create function text_to_artifacts(artifacts text) returns artifact[]
    language sql
    immutable as
$$
select
            array_agg(omni_manifest.text_to_artifact(artifact)) filter (where length(artifact) > 0)
from
    regexp_split_to_table(artifacts, '\s*(\n+|;)\s*') r(artifact)
$$;