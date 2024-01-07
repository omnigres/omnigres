create type http_header as
(
    name  text,
    value text
);

create domain http_headers as http_header[];

/*{% include "../src/http_header.sql" %}*/
/*{% include "../src/http_header_get_all.sql" %}*/
/*{% include "../src/http_header_get.sql" %}*/

