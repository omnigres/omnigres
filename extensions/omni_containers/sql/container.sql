-- Creating container
SELECT count(*) FROM pg_catalog.pg_tables WHERE tablename = 'container_test';
SET client_min_messages TO WARNING;
WITH container AS (SELECT * FROM omni_containers.docker_container_create('ghcr.io/yrashk/psql', cmd => $$psql -c 'CREATE TABLE container_test ();'$$,
                                                      wait => true, pull => true) AS id)
SELECT omni_containers.docker_container_inspect(id)->'State'->'ExitCode' AS exit_code,
       omni_containers.docker_container_logs(id) AS logs
       FROM container;
SET client_min_messages TO NOTICE;
SELECT count(*) FROM pg_catalog.pg_tables WHERE tablename = 'container_test';

-- Passing extra options
WITH container AS (SELECT * FROM omni_containers.docker_container_create('ghcr.io/yrashk/psql', cmd => $$echo $TEST$$,
                                                      options => $${"Env": ["TEST=ok"]}$$,
                                                      wait => true, pull => true) AS id)
SELECT omni_containers.docker_container_inspect(id)->'State'->'ExitCode' AS exit_code,
       omni_containers.docker_container_logs(id) AS logs
       FROM container;

-- Creating a container that can't be found
SELECT true FROM omni_containers.docker_container_create('cannotbefound');