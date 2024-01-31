create function artifact_to_text(artifact artifact) returns text
    language sql
    immutable as
$$
select
    artifact.self::text || case
                               when artifact.requirements is null then ''
                               else '#' || artifact.requirements::text end
$$;