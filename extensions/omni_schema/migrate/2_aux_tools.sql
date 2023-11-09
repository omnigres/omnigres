create table auxiliary_tools
(
    id                  integer primary key generated always as identity,
    filename_stem       text,
    filename_extension  text,
    processor           name not null,
    processor_extension name,
    priority            integer not null default 0
);

insert
into
    auxiliary_tools (filename_stem, filename_extension, processor, processor_extension, priority)
values
    ('requirements', 'txt', 'omni_python.install_requirements', 'omni_python', 1);

select pg_catalog.pg_extension_config_dump('auxiliary_tools', '');
