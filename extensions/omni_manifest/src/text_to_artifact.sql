create function text_to_artifact(artifact text) returns artifact
    language sql
    immutable as
$$
with
    arr as (select regexp_split_to_array(artifact, '#') val)
select
    row (arr.val[1]::omni_manifest.requirement, arr.val[2]::omni_manifest.requirement[])::omni_manifest.artifact
from
    arr
$$;