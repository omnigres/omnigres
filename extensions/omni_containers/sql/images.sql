-- Ensure we pull an image
SET client_min_messages TO WARNING;
WITH container AS (SELECT * FROM omni_containers.docker_container_create('ghcr.io/yrashk/psql', cmd => $$true$$,
                                                      wait => true, pull => true) AS id)
SELECT true FROM container;
SET client_min_messages TO NOTICE;

\x ON
SELECT true FROM omni_containers.docker_images WHERE 'ghcr.io/yrashk/psql:latest' = any(repo_tags);