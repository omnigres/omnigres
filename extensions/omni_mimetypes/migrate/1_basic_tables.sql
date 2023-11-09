create type mime_type_source as enum ('iana', 'apache', 'nginx');

create table mime_types
(
    id           integer primary key generated always as identity,
    name         text not null unique,
    source       mime_type_source,
    compressible bool,
    charset      text
);

create table file_extensions
(
    id        integer primary key generated always as identity,
    extension text not null unique
);

create table mime_types_file_extensions
(
    mime_type_id      integer references mime_types (id),
    file_extension_id integer references file_extensions (id)
);


select pg_catalog.pg_extension_config_dump('mime_types', '');
select pg_catalog.pg_extension_config_dump('mime_types_id_seq', '');
select pg_catalog.pg_extension_config_dump('file_extensions', '');
select pg_catalog.pg_extension_config_dump('file_extensions_id_seq', '');
select pg_catalog.pg_extension_config_dump('mime_types_file_extensions', '');