-- API: PUBLIC
create function docker_container_create(
    image text,
    cmd text default null,
    attach text default 'db.omni',
    start bool default true,
    wait bool default false,
    pull bool default false,
    options jsonb default '{}')
    returns text
as
'MODULE_PATHNAME',
'docker_container_create'
    language c;