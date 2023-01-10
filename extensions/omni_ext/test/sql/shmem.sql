-- Since it's preloaded, it's already initialized
SELECT omni_ext_test.alloc_shmem_global();

--- Database-local
SELECT omni_ext_test.alloc_shmem_database_local();

CREATE DATABASE another_db;
\connect another_db
CREATE EXTENSION omni_ext_test CASCADE;
SELECT omni_ext_test.alloc_shmem_database_local();