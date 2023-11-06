create table languages
(
    id                       integer primary key generated always as identity,
    file_extension           text not null unique,
    language                 name not null,
    extension                name,
    file_processor           name,
    file_processor_extension name
);

insert
into
    languages (file_extension, language, extension, file_processor, file_processor_extension)
values
    ('sql', 'sql', null, null, null),
    ('pl', 'plperlu', 'plperlu', null, null),
    ('trusted.pl', 'plperl', 'plperl', null, null),
    ('py', 'plpython3u', 'plpython3u', 'omni_python.create_functions', 'omni_python'),
    ('tcl', 'pltclu', 'pltclu', null, null),
    ('trusted.tcl', 'pltcl', 'pltcl', null, null),
    ('rs', 'plrust', 'plrust', null, null);


select pg_catalog.pg_extension_config_dump('languages', '');