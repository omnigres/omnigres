create function requirement_to_json(requirement requirement) returns json
    language sql
    immutable as
$$
select json_build_object(requirement.name, requirement.version)
$$;