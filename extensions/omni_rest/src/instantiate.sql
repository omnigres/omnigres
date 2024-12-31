create or replace function instantiate(schema name default 'omni_rest')
    returns void
    language plpgsql
as
$instantiate$
begin
    perform set_config('search_path', schema::text || ',public', true);

    create type postgrest_settings as
    (
        -- The only schemas `postgrest` is allowed to access
        schemas name[]
    );

    /*{% include "postgrest_settings.sql" %}*/
    /*{% include "_postgrest_relation.sql" %}*/
    /*{% include "_postgrest_function.sql" %}*/

    /*{% include "__eval.sql" %}*/
    /*{% include "postgrest_get.sql" %}*/
    /*{% include "postgrest_rpc.sql" %}*/
    /*{% include "postgrest_insert.sql" %}*/
    /*{% include "postgrest_update.sql" %}*/
    /*{% include "postgrest_cors.sql" %}*/
    /*{% include "postgrest.sql" %}*/
end;
$instantiate$;