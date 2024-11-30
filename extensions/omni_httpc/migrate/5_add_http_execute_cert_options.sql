alter type http_execute_options add attribute allow_self_signed_cert boolean;

alter type http_execute_options add attribute cacerts text[];

create type client_certificate
as
(
    certificate text,
    private_key text
);

alter type http_execute_options add attribute clientcert client_certificate;

drop function http_execute_options;
/*{% include "../src/http_execute_options.sql" %}*/
