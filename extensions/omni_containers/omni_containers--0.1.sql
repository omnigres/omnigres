-- API: PRIVATE
create function docker_images_json()
    returns jsonb
as
'MODULE_PATHNAME',
'docker_images_json'
    language c;

comment
    on function docker_images_json() is 'Private API';

-- API: PUBLIC
create view docker_images as
    (
    select
        "Id"                    as id,
        "Size"                  as size,
        "Labels"                as labels,
        to_timestamp("Created") as created_at,
        "ParentId"              as parent_id,
        "RepoTags"              as repo_tags,
        "Containers"            as containers,
        "SharedSize"            as shared_size,
        "RepoDigests"           as repo_digests,
        "VirtualSize"           as virtual_size
    from
        jsonb_to_recordset(jsonb_strip_nulls(omni_containers.docker_images_json()))
            as images("Id" text, "Size" int8, "Labels" jsonb, "Created" int8, "ParentId" text, "RepoTags" text[],
                      "Containers" int, "SharedSize" int, "RepoDigests" text[], "VirtualSize" jsonb)
        );

create type docker_container_environment_variable as
(
    key   text,
    value text
);

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

-- API: PUBLIC
create function docker_container_inspect(id text)
    returns jsonb
as
'MODULE_PATHNAME',
'docker_container_inspect'
    language c;

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

-- API: PUBLIC
create function docker_container_stop(id text)
    returns void
as
'MODULE_PATHNAME',
'docker_container_stop'
    language c;
