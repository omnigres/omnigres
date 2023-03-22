-- Creating container
select
    count(*)
from
    pg_catalog.pg_tables
where
    tablename = 'container_test';
set client_min_messages to WARNING;
with
    container as (select *
                  from
                      omni_containers.docker_container_create('ghcr.io/yrashk/psql',
                                                              cmd => $$psql -c 'CREATE TABLE container_test ();'$$,
                                                              wait => true, pull => true) as id)
select
    omni_containers.docker_container_inspect(id) -> 'State' -> 'ExitCode' as exit_code,
    omni_containers.docker_container_logs(id)                             as logs
from
    container;
set client_min_messages to NOTICE;
select
    count(*)
from
    pg_catalog.pg_tables
where
    tablename = 'container_test';

-- Passing extra options
with
    container as (select *
                  from
                      omni_containers.docker_container_create('ghcr.io/yrashk/psql', cmd => $$echo $TEST$$,
                                                              options => $${"Env": ["TEST=ok"]}$$,
                                                              wait => true, pull => true) as id)
select
    omni_containers.docker_container_inspect(id) -> 'State' -> 'ExitCode' as exit_code,
    omni_containers.docker_container_logs(id)                             as logs
from
    container;

-- Creating a container that can't be found
select
    true
from
    omni_containers.docker_container_create('cannotbefound');