create function activate_policy(policy_id int)
    returns table
            (
                stmt  text,
                error text
            )
    language plpgsql as
$pgsql$
declare
    policy omni_access.policies%rowtype;
    rec record;
begin
    -- Get the policy
    select * from omni_access.policies where policies.id = policy_id into policy;

    -- Sanity checks
    if not found then
        raise exception 'Policy % not found', policy_id;
    end if;
    if policy.active then
        raise warning 'Policy % is already active', policy_id;
        return;
    end if;

    for rec in select * from omni_access.policy_activation(policy_id)
        loop
            begin
                stmt := rec.stmt;
                execute rec.stmt;
            exception
                when others then
                    error := sqlerrm;
                    return next;
                    return;
            end;
            return next;
        end loop;


    return;
end;
$pgsql$;