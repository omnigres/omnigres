-- API: PUBLIC
create function docker_container_stop(id text)
    returns void
as
'MODULE_PATHNAME',
'docker_container_stop'
    language c;
