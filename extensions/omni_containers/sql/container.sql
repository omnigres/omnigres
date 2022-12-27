-- Creating container
SELECT count(*) FROM pg_catalog.pg_tables WHERE tablename = 'container_test';
SET client_min_messages TO WARNING;
SELECT true FROM omni_containers.docker_container_create('governmentpaas/psql', cmd => $$psql -c 'CREATE TABLE container_test ();'$$,
                                                      wait => true, pull => true);
SET client_min_messages TO NOTICE;
SELECT count(*) FROM pg_catalog.pg_tables WHERE tablename = 'container_test';

-- Creating a container that can't be found
SELECT true FROM omni_containers.docker_container_create('cannotbefound');