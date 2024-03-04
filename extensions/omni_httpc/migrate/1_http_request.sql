create type http_request as
(
    method      omni_http.http_method,
    url         text,
    headers     omni_http.http_headers,
    body bytea
);


/*{% include "../src/http_request.sql" %}*/
