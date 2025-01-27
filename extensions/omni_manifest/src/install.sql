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
            e.name                                                      as ext_name,
            case when r.version = '*' then e.version else r.version end as target_version,
            pg_extension.extversion                                     as installed_version
        from
            unnest(plan) with ordinality r(name, version, ord)
            left outer join pg_extension on pg_extension.extname = r.name
            left outer join lateral
                (select name, version from pg_available_extension_versions where r.version != '*'
                 union select name, default_version as version from pg_available_extensions where r.version = '*') e
                on e.name = r.name and (e.version = r.version or r.version = '*')
        order by ord
        loop
            if rec.ext_name is null then
                missing_requirements = true;
                return next row (rec.req, 'missing')::omni_manifest.install_report;
            else
                if rec.target_version = rec.installed_version then
                    return next row (rec.req, 'kept')::omni_manifest.install_report;
                    continue;
                end if;
                if not missing_requirements then

                    if rec.installed_version is null then
                        execute format('create extension %s version %L', (rec.req).name, rec.target_version);
                        return next row (rec.req, 'installed')::omni_manifest.install_report;
                    else
                        execute format('alter extension %s update to %L', (rec.req).name, rec.target_version);
                        return next row (rec.req, 'updated')::omni_manifest.install_report;
                    end if;
                end if;
            end if;
        end loop;
    return;
end;
$$;