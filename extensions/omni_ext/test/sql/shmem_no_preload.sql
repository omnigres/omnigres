-- Since it's not preloaded, this test should not pass
SELECT omni_ext_test_no_preload.alloc_shmem_global();
SELECT omni_ext_test_no_preload.alloc_shmem_database_local();

-- Load
SELECT omni_ext.load('omni_ext_test_no_preload');

-- Should work now
SELECT omni_ext_test_no_preload.alloc_shmem_global();
-- Should work now, too
SELECT omni_ext_test_no_preload.alloc_shmem_database_local();
CREATE DATABASE another_db;
\connect another_db
CREATE EXTENSION omni_ext_test_no_preload CASCADE;
SELECT omni_ext_test_no_preload.alloc_shmem_database_local();