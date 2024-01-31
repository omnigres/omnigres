create function requirement_to_text(requirement requirement) returns text
    language sql
    immutable as
$$
select
        requirement.name || case
                                when left(requirement.version, 1) = '='
                                    then requirement.version
                                else '=' || requirement.version end;
$$;