create function materialize_meta(
    from_schema regnamespace,
    to_schema regnamespace) returns void
    language plpgsql
as
$create_remote_meta$
declare
begin
    perform set_config('search_path', to_schema || ',' || from_schema, true);

    create materialized view if not exists "cast" as
        select * from "cast" with no data;
    create materialized view if not exists "column" as
        select * from "column" with no data;
    create materialized view if not exists "connection" as
        select * from "connection" with no data;
    create materialized view if not exists "constraint_check" as
        select * from "constraint_check" with no data;
    create materialized view if not exists "constraint_unique" as
        select * from "constraint_unique" with no data;
    create materialized view if not exists "extension" as
        select * from "extension" with no data;
    create materialized view if not exists "foreign_column" as
        select * from "foreign_column" with no data;
    create materialized view if not exists "foreign_data_wrapper" as
        select * from "foreign_data_wrapper" with no data;
    create materialized view if not exists "foreign_key" as
        select * from "foreign_key" with no data;
    create materialized view if not exists "foreign_server" as
        select * from "foreign_server" with no data;
    create materialized view if not exists "foreign_table" as
        select * from "foreign_table" with no data;
    create materialized view if not exists "function" as
        select * from "function" with no data;
    create materialized view if not exists "function_parameter" as
        select * from "function_parameter" with no data;
    create materialized view if not exists "operator" as
        select * from "operator" with no data;
    create materialized view if not exists "policy" as
        select * from "policy" with no data;
    create materialized view if not exists "policy_role" as
        select * from "policy_role" with no data;
    create materialized view if not exists "relation" as
        select * from "relation" with no data;
    create materialized view if not exists "relation_column" as
        select * from "relation_column" with no data;
    create materialized view if not exists "role" as
        select * from "role" with no data;
    create materialized view if not exists "role_inheritance" as
        select * from "role_inheritance" with no data;
    create materialized view if not exists "schema" as
        select * from "schema" with no data;
    create materialized view if not exists "sequence" as
        select * from "sequence" with no data;
    create materialized view if not exists "table" as
        select * from "table" with no data;
    create materialized view if not exists "table_privilege" as
        select * from "table_privilege" with no data;
    create materialized view if not exists "trigger" as
        select * from "trigger" with no data;
    create materialized view if not exists "type" as
        select * from "type" with no data;
    create materialized view if not exists "view" as
        select * from "view" with no data;

    create function refresh_meta() returns void
        language plpgsql
    as
    $refresh_meta$
    begin
        refresh materialized view "cast";
        refresh materialized view "column";
        refresh materialized view "connection";
        refresh materialized view "constraint_check";
        refresh materialized view "constraint_unique";
        refresh materialized view "extension";
        refresh materialized view "foreign_column";
        refresh materialized view "foreign_data_wrapper";
        refresh materialized view "foreign_key";
        refresh materialized view "foreign_server";
        refresh materialized view "foreign_table";
        refresh materialized view "function";
        refresh materialized view "function_parameter";
        refresh materialized view "operator";
        refresh materialized view "policy";
        refresh materialized view "policy_role";
        refresh materialized view "relation";
        refresh materialized view "relation_column";
        refresh materialized view "role";
        refresh materialized view "role_inheritance";
        refresh materialized view "schema";
        refresh materialized view "sequence";
        refresh materialized view "table";
        refresh materialized view "table_privilege";
        refresh materialized view "trigger";
        refresh materialized view "type";
        refresh materialized view "view";
    end;
    $refresh_meta$;
    execute format('alter function refresh_meta() set search_path = %s', to_schema || ',' || from_schema);

    perform refresh_meta();
    return;
end;
$create_remote_meta$;