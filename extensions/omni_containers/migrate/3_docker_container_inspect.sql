-- API: PUBLIC
create function docker_container_inspect(id text)
    returns jsonb
as
'MODULE_PATHNAME',
'docker_container_inspect'
    language c;
