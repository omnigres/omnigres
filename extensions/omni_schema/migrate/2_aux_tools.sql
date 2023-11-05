create table auxiliary_tools
(
    id                  integer primary key generated always as identity,
    filename_stem       text,
    filename_extension  text,
    processor           name not null,
    processor_extension name
);

insert
into
    auxiliary_tools (filename_stem, filename_extension, processor, processor_extension)
values
    ('requirements', 'txt', 'omni_python.install_requirements', 'omni_python');

select pg_catalog.pg_extension_config_dump('auxiliary_tools', '');
