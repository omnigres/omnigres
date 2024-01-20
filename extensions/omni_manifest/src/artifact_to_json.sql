create function artifact_to_json(artifact artifact) returns json
    language sql
    immutable as
$$
select
    json_build_object('target', artifact.self::json,
                      'requirements', artifact.requirements::json)
$$;