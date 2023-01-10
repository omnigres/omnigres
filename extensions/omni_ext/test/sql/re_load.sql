-- Pre-loaded value
SELECT omni_ext_test.alloc_shmem_global();

-- Change it
SELECT omni_ext_test.update_global_value('updated');

-- Re-load again
SELECT omni_ext.load('omni_ext_test');

-- Should not be rolled back to the initialized value
SELECT omni_ext_test.alloc_shmem_global();