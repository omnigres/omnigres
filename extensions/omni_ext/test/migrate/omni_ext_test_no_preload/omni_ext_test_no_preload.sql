CREATE FUNCTION alloc_shmem_global()
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'alloc_shmem_global'
    LANGUAGE C;

CREATE FUNCTION alloc_shmem_database_local()
    RETURNS cstring
    AS 'MODULE_PATHNAME', 'alloc_shmem_database_local'
    LANGUAGE C;