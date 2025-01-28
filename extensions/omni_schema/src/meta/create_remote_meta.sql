create function create_remote_meta(
    schema regnamespace,
    remote_schema name,
    server name) returns void
    language plpgsql
as
$create_remote_meta$
declare
begin
    execute format(
            'import foreign schema %I limit to ("schema","cast","operator","sequence","table","view","relation_column","column","relation","function","function_info_schema","function_parameter","trigger","role","role_inheritance","table_privilege","policy","policy_role","connection","constraint_unique","constraint_check","extension","foreign_data_wrapper","foreign_server","foreign_table","foreign_column") from server %I into %I',
            remote_schema, server, schema);
    return;
end;
$create_remote_meta$;