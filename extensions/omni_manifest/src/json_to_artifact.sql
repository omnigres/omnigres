create function json_to_artifact(artifact json) returns artifact
    language sql
    immutable as
$$
select
    row ((artifact -> 'target')::omni_manifest.requirement, (artifact -> 'requirements')::omni_manifest.requirement[])::omni_manifest.artifact
$$;