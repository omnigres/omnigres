-- API: PRIVATE
create function docker_stream_to_text(
    stream bytea
)
    returns text
as
'MODULE_PATHNAME',
'docker_stream_to_text'
    language c;


/*{% include "../src/docker_container_logs.sql" %}*/