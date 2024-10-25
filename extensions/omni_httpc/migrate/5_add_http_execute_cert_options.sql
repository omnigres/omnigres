alter type http_execute_options add attribute allow_self_signed_cert boolean;

alter type http_execute_options add attribute cacerts text[];

drop function http_execute_options;
/*{% include "../src/http_execute_options.sql" %}*/
