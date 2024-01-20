create function artifact_to_jsonb(artifact artifact) returns jsonb
    language sql
    immutable as
$$
select
    jsonb_build_object('target', artifact.self::jsonb,
                       'requirements', artifact.requirements::jsonb)
$$;