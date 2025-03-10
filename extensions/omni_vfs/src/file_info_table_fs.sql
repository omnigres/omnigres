CREATE OR REPLACE FUNCTION omni_vfs.file_info(fs omni_vfs.table_fs, path TEXT) RETURNS omni_vfs_types_v1.file_info
    STABLE
    LANGUAGE sql SET search_path TO omni_vfs
AS
$$

WITH
    match(id) AS (
        SELECT omni_vfs.table_fs_file_id(fs, path)
        UNION ALL
        SELECT NULL WHERE path = '/'
    )
SELECT
   COALESCE(LENGTH(d.data), 0) AS size,
    COALESCE(d.created_at, NOW()) AS created_at,
    COALESCE(d.accessed_at, NOW()) AS accessed_at,
    COALESCE(d.modified_at, NOW()) AS modified_at,
    COALESCE(f.kind, 'dir') AS kind
FROM
    omni_vfs.table_fs_files                f
    FULL OUTER JOIN match         m ON f.id = m.id
    FULL OUTER JOIN omni_vfs.table_fs_file_data d ON m.id = d.file_id
WHERE
 (f.filesystem_id = fs.id OR m.id IS NULL);

$$;
