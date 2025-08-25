/*{% include "../src/_validate_connstr.sql" %}*/
/*{% include "../src/dump.sql" %}*/
/*{% include "../src/restore.sql" %}*/

do
$$
    begin
        perform from pg_roles where rolname = 'omni_schema_external_tool_caller';
        if found then
            return;
        end if;
        create role omni_schema_external_tool_caller nologin;
        grant pg_execute_server_program to omni_schema_external_tool_caller;
        grant usage on schema omni_schema to omni_schema_external_tool_caller;
        grant execute on function pg_config to omni_schema_external_tool_caller;

        alter function _dump owner to omni_schema_external_tool_caller;
        alter function _restore owner to omni_schema_external_tool_caller;

    end;
$$;
