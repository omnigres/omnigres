create function install_plan(artifacts artifact[])
    returns requirement[]
    language plpgsql
as
$$
declare
    plan omni_manifest.requirement[];
    artifact             omni_manifest.artifact;
    req                  omni_manifest.requirement;
    rec  record;
    _id  integer;
    _id_ integer;
    c    int;
begin
    create temporary table omni_manifest__deps__
    (
        id                       integer primary key generated always as identity,
        requirement              omni_manifest.requirement unique,
        outstanding_dependencies int
    ) on
          commit drop;

    create temporary table omni_manifest__parents__
    (
        child  int,
        parent int,
        constraint u unique (child, parent)
    )
        on commit drop;
    foreach artifact in array artifacts
        loop
            insert
            into
                omni_manifest__deps__ (requirement, outstanding_dependencies)
            values ((artifact).self, array_length((artifact).requirements, 1))
            on conflict (requirement) do update set
                outstanding_dependencies = array_length((artifact).requirements, 1)
            returning id into _id;

            foreach req in array (artifact).requirements
                loop
                    insert
                    into
                        omni_manifest__deps__ (requirement, outstanding_dependencies)
                    values (req, 0)
                    on conflict (requirement) do update set requirement = excluded.requirement
                    returning id
                        into _id_;
                    insert
                    into
                        omni_manifest__parents__
                    values (_id_, _id)
                    on conflict (child, parent) do nothing;
                end loop;

        end loop;

    c := 1;
    while c > 0
        loop
            select count(*) from omni_manifest__deps__ where omni_manifest__deps__.outstanding_dependencies > 0 into c;
            for rec in select * from omni_manifest__deps__ order by (requirement).name, (requirement).version asc
        loop
                    if rec.outstanding_dependencies = 0 then
                        plan = array_append(plan, rec.requirement);
                        delete from omni_manifest__deps__ where omni_manifest__deps__.id = rec.id;
                        update omni_manifest__deps__
                        set
                            outstanding_dependencies = outstanding_dependencies - 1
                        from
                            omni_manifest__parents__
                        where
                            parent = id and
                            child = rec.id;
                    end if;
                end loop;
        end loop;

    drop table omni_manifest__deps__;
    drop table omni_manifest__parents__;

    return plan;
end;
$$;