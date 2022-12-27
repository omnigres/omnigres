-- API: PRIVATE
CREATE FUNCTION docker_images_json()
    RETURNS jsonb
    AS 'MODULE_PATHNAME', 'docker_images_json'
    LANGUAGE C;

COMMENT ON FUNCTION docker_images_json() IS 'Private API';

-- API: PUBLIC
CREATE VIEW docker_images AS (
     select "Id" AS id, "Size" AS "size", "Labels" AS "labels", to_timestamp("Created") AS created_at,
            "ParentId" AS parent_id, "RepoTags" AS repo_tags, "Containers" AS containers,
            "SharedSize" AS shared_size, "RepoDigests" AS repo_digests, "VirtualSize" AS virtual_size FROM
            jsonb_to_recordset(jsonb_strip_nulls(omni_containers.docker_images_json())) AS
              images("Id" text, "Size" int8, "Labels" jsonb, "Created" int8, "ParentId" text, "RepoTags" text[],
                     "Containers" int, "SharedSize" int, "RepoDigests" text[], "VirtualSize" jsonb)
);

CREATE TYPE docker_container_environment_variable AS (
  key text,
  value text
);

-- API: PUBLIC
CREATE FUNCTION docker_container_create(
  image text,
  cmd text DEFAULT NULL,
  attach text DEFAULT 'db.omni',
  start bool DEFAULT true,
  wait bool DEFAULT false,
  pull bool DEFAULT false,
  options jsonb DEFAULT '{}')
RETURNS text
AS 'MODULE_PATHNAME', 'docker_container_create'
    LANGUAGE C;

-- API: PUBLIC
CREATE FUNCTION docker_container_inspect(id text)
RETURNS jsonb
AS 'MODULE_PATHNAME', 'docker_container_inspect'
    LANGUAGE C;

-- API: PUBLIC
CREATE FUNCTION docker_container_logs(
  id text,
  stdout bool DEFAULT true,
  stderr bool DEFAULT true,
  since timestamp DEFAULT NULL,
  until timestamp DEFAULT NULL,
  timestamps bool DEFAULT false,
  tail int DEFAULT NULL
  )
RETURNS text
AS 'MODULE_PATHNAME', 'docker_container_logs'
    LANGUAGE C;