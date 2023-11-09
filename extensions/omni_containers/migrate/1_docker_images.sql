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
