-- Ensure we pull an image
set client_min_messages to WARNING;
with
    container as (select *
                  from
                      omni_containers.docker_container_create('ghcr.io/yrashk/psql', cmd => $$true$$,
                                                              wait => true, pull => true) as id)
select
    true as result
from
    container;
set client_min_messages to NOTICE;

\x on
select
    true as result
from
    omni_containers.docker_images
where
    'ghcr.io/yrashk/psql:latest' = any (repo_tags);