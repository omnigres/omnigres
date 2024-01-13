create function requirement(name text, version text) returns requirement
    immutable
    language sql
as
$$
select row (name, version)::omni_manifest.requirement
$$;