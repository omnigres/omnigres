/*{% extends "meta_views.sql" %}*/

/*{% block content %}*/
create function instantiate_evolution(schema name) returns void
    language plpgsql
as
$instantiate_evolution$
declare
    -- Include omni_schema for other functionality
    -- Include omni_polyfill to ensure we have uuidv7
    -- Include omni_vfs to access file systems
    -- Include public for dblink
    search_path text := 'omni_schema,omni_vfs,omni_polyfill,public,pg_catalog';
begin
    if schema != 'omni_schema' then
        search_path := schema || ',' || search_path;
    end if;
    perform set_config('search_path', search_path, true);

    create domain revision_id as uuid check ( uuid_extract_version(value) = 7);

    /*{% include "../src/evolution/assemble_schema_revision.sql" %}*/
    execute format('alter function assemble_schema_revision set search_path = %s', search_path);

    /*{% include "../src/evolution/capture_schema_revision.sql" %}*/
    execute format('alter function capture_schema_revision set search_path = %s', search_path);

    /*{% include "../src/evolution/schema_revisions.sql" %}*/
    execute format('alter function schema_revisions set search_path = %s', search_path);

    /*{% include "../src/evolution/migrate_to_schema_revision.sql" %}*/
    execute format('alter function migrate_to_schema_revision set search_path = %s', search_path);

end;
$instantiate_evolution$;
/*{% endblock %}*/
