create type fallback_route as
(
    request omni_httpd.http_request
);

/*{% include "../src/fallback_route.sql" %}*/
/*{% include "../src/handler_fallback_route.sql" %}*/

create type postgrest_get_route as
(
    request  omni_httpd.http_request,
    relation regclass
);


/*{% include "../src/postgrest_get_route.sql" %}*/
/*{% include "../src/handler_postgrest_get_route.sql" %}*/

create type postgrest_cors_route as
(
    request omni_httpd.http_request,
    origin  text
);

/*{% include "../src/postgrest_cors_route.sql" %}*/
/*{% include "../src/handler_postgrest_cors_route.sql" %}*/


/*{% include "../src/postgrest.sql" %}*/