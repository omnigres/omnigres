create function requirement_to_jsonb(requirement requirement) returns jsonb
    language sql
    immutable as
$$
select jsonb_build_object(requirement.name, requirement.version)
$$;