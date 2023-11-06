create table migrations
(
    id         integer primary key generated always as identity,
    name       text      not null,
    migration  text      not null,
    applied_at timestamp not null default now()
);

select pg_catalog.pg_extension_config_dump('migrations', '');
select pg_catalog.pg_extension_config_dump('migrations_id_seq', '');