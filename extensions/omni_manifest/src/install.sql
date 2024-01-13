create function install(artifacts artifact[]) returns setof install_report
    language plpgsql
as
$$
declare
    rec record;
    missing_requirements bool = false;
    plan omni_manifest.requirement[];
begin
    plan = omni_manifest.install_plan(artifacts);
    -- Check if we're missing some artifacts
    for rec in
        select
            row (r.name, r.version)::omni_manifest.requirement as req,
            e.name                                             as ext_name
        from
            unnest(plan) with ordinality r(name, version, ord)
            left outer join
                pg_available_extension_versions e
                on e.name = r.name and e.version = r.version
        order by ord
        loop
            if rec.ext_name is null then
                missing_requirements = true;
                return next row (rec.req, 'missing')::omni_manifest.install_report;
            else
                if not missing_requirements then
                    execute format('create extension %s version %L', (rec.req).name, (rec.req).version);
                    return next row (rec.req, 'installed')::omni_manifest.install_report;
                end if;
            end if;
        end loop;
    if missing_requirements then
        return;
    end if;
    -- Now figure out the order of these dependencies (topological sort)
    return;
end;
$$;