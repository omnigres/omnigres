-- API: PRIVATE
create function docker_api_base_url()
    returns text
as
'MODULE_PATHNAME',
'docker_api_base_url'
    language c;

-- API: PRIVATE
create function docker_host_ip()
    returns text
as
'MODULE_PATHNAME',
'docker_host_ip'
    language c;

-- API: PRIVATE
create function docker_images_json()
    returns jsonb
as
$$
declare
response omni_httpc.http_response;
begin
    select * into response from omni_httpc.http_execute(
        omni_httpc.http_request(
            format('http://%s/images/json', omni_containers.docker_api_base_url())
        )
    );
    if response.status = 200 then
        return convert_from(response.body, 'UTF8');
    else
        raise exception 'Can''t list docker images' using
            detail = format('Error code %s: %s', response.status, convert_from(response.body, 'UTF8')::jsonb);
    end if;
end;
$$ language plpgsql;

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
