create function jsonb_to_artifact(artifact jsonb) returns artifact
    language sql
    immutable as
$$
select
    row ((artifact -> 'target')::omni_manifest.requirement, (artifact -> 'requirements')::omni_manifest.requirement[])::omni_manifest.artifact
$$;