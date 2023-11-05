create type config_key as enum ( 'site_packages', 'extra_pip_index_url', 'pip_find_links' );
create table config
(
    id    integer primary key generated always as identity,
    name config_key not null,
    value text not null
);