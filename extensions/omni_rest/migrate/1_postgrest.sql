create type postgrest_settings as
(
    -- The only schemas `postgrest` is allowed to access
    schemas name[]
);

/*{% include "../src/postgrest_settings.sql" %}*/
/*{% include "../src/_postgrest_relation.sql" %}*/

/*{% include "../src/__eval.sql" %}*/
/*{% include "../src/postgrest_get.sql" %}*/
/*{% include "../src/postgrest_insert.sql" %}*/
/*{% include "../src/postgrest_cors.sql" %}*/
/*{% include "../src/postgrest.sql" %}*/