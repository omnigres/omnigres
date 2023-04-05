alter system set max_worker_processes = 64;
alter system set shared_preload_libraries = 'omni_ext--0.1', 'plrust';