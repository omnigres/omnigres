SELECT omni_ext_test.wait_for_table('local_worker_started');
\d local_worker_started;

CREATE DATABASE another_db_local_worker;
\connect another_db_local_worker
CREATE EXTENSION omni_ext_test CASCADE;
SELECT omni_ext_test.wait_for_table('local_worker_started');
\d local_worker_started;