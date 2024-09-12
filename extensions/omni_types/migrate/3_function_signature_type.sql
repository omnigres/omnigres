create table function_signature_types
(
    typ       regtype primary key unique,
    arguments regtype[] not null,
    return    regtype   not null
);

select pg_catalog.pg_extension_config_dump('function_signature_types', '');

/*{% include "../src/function_signature_type_of.sql" %}*/

/*{% include "../src/function_signature_type.sql" %}*/