create function check_if_role_accessible_to_current_user(role name) returns boolean
as
$$
declare
    result boolean;
begin
    with
        recursive
        role_members as (select
                             roleid,
                             member
                         from
                             pg_auth_members
                         where
                             roleid = (select oid from pg_roles where rolname = role)

                         union all

                         select
                             am.roleid,
                             am.member
                         from
                             pg_auth_members  am
                             join
                                 role_members rm on am.roleid = rm.member)
    select
        true
    into result
    from
        pg_roles r
    where
        (r.rolname = current_user and r.rolsuper) or
        (r.oid in (select member from role_members)) or
        (r.rolname = role and role = current_user)
    limit 1;
    if not found then
        return false;
    else
        return true;
    end if;
end;

$$ language plpgsql;
