-- API: PUBLIC
create function docker_container_logs(
    id text,
    stdout bool default true,
    stderr bool default true,
    since timestamp default null,
    until timestamp default null,
    timestamps bool default false,
    tail int default null
)
    returns text
as
'MODULE_PATHNAME',
'docker_container_logs'
    language c;