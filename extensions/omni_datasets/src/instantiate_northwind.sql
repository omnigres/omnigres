create function instantiate_northwind(schema regnamespace) returns void
    language plpgsql
as
$instantiate_northwind$
declare
    old_search_path text := current_setting('search_path');
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    /*{% include "northwind_ddl.sql" %}*/
    /*{% include "northwind_data.sql" %}*/

    -- Restore the path
    perform set_config('search_path', old_search_path, true);
end
$instantiate_northwind$;
