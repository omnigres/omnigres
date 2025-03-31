# omni_vfs

This extension provides a unified Virtual File System (VFS) API for Postgres to interact with different file systems regardless of their backends.

This approach allows interchangeable backends in the following scenarios:

* **Interacting with files in development environment**

This enables a smooth development experience that doesn't require moving data from the filesystem to the database, like serving static files, picking up migrations, etc.

* **Retrieving files from a Postgres-backed Git repositories**

This can facilitate streamline deployment rollouts as migrations and static files can be retrieved using the same API.

* **Interacting with remote storages**

A lot of applications deal with remote storages like S3 or Google Cloud Storage. This interface allows using local file system when testing and a real one in staging and production, without changing anything.

!!! warning "Work in progress"

    This extension's API is minimalistic at this time and does not support a number of scenarios.
    For example, there are no writing capabilities, any form of streaming, etc.

    It is intended that this functionality will be implemented. You can also help by [contributing](https://github.com/omnigres/omnigres)
    or funding the development of features that you need.

## Example

The following query:

```postgresql
with
    fs as (select omni_vfs.local_fs('/home/omni/dev/omnigres/omnigres/extensions/omni_vfs') as fs),
    entries as (select fs, (omni_vfs.list(fs, '')).* from fs)
select
    entries.name,
    entries.kind,
    (omni_vfs.file_info(fs, '/' || entries.name)).*
from
    entries;
```

Results in:

|       name        | kind | size |         created_at         |        accessed_at         |        modified_at         |
|-------------------|------|------|----------------------------|----------------------------|----------------------------|
| CMakeLists.txt    | file |  429 | 2023-06-08 14:10:18.742915 | 2023-06-08 14:10:19.407029 | 2023-06-08 14:10:18.742915 |
| mkdocs.yml        | file |   50 | 2023-06-08 14:10:18.743315 | 2023-06-08 14:10:19.3241   | 2023-06-08 14:10:18.743315 |
| omni_vfs.h        | file |  229 | 2023-06-08 14:10:18.743582 | 2023-06-08 14:10:19.407321 | 2023-06-08 14:10:18.743582 |
| tests             | dir  |   96 | 2023-06-08 14:10:18.743633 | 2023-06-08 14:10:20.495602 | 2023-06-08 14:10:18.743633 |
| docs              | dir  |   96 | 2023-06-08 17:24:37.978643 | 2023-06-08 17:24:39.164254 | 2023-06-08 17:24:37.978643 |
| local_fs.c        | file | 9371 | 2023-06-08 14:39:41.67565  | 2023-06-08 14:40:18.467411 | 2023-06-08 14:39:41.67565  |
| README.md         | file |   12 | 2023-06-08 14:10:18.743    | 2023-06-08 14:10:19.407083 | 2023-06-08 14:10:18.743    |
| omni_vfs--0.1.sql | file | 3073 | 2023-06-08 17:10:55.278702 | 2023-06-08 17:11:04.599535 | 2023-06-08 17:10:55.278702 |
| omni_vfs.c        | file |  347 | 2023-06-08 14:10:18.743496 | 2023-06-08 14:10:19.40728  | 2023-06-08 14:10:18.743496 |

## API

## `omni_vfs_types_v1.file` type

Describes a file entry.

|    Field | Type                     | Description                                   |
|---------:|--------------------------|-----------------------------------------------|
| **name** | `text`                   | File name                                     |
| **kind** | `omni_vfs_types_v1.file_kind` | File kind (`file`, `dir`) [^other-file-types] |

## `omni_vs_api.file_info` type

Describes file meta information.

|           Field | Type                     | Description                                   |
|----------------:|--------------------------|-----------------------------------------------|
|        **size** | `bigint`                 | File size                                     |
|  **created_at** | `timestamp`              | File creation time (if available)             |
| **accessed_at** | `timestamp`              | File access time (if available)               |
| **modified_at** | `timestamp`              | File modification time (if available)         |
|        **kind** | `omni_vfs_types_v1.file_kind` | File kind (`file`, `dir`) [^other-file-types] |

## `omni_vfs.list()`

Lists a directory or a single file.

|            Parameter | Type              | Description                                                       |
|---------------------:|-------------------|-------------------------------------------------------------------|
|               **fs** | _Filesystem type_ | Filesystem                                                        |
|             **path** | `text`            | Path to list. If it is a single file, returns that file           |
| **fail_unpermitted** | `bool`            | Raise an error if directory can't be open. `true` **by default**. |

Returns a set of `omni_vfs_types_v1.file` values.

## `omni_vfs.list_recursively()`

This is a helper function implemented for all backends that lists all files recursively.

| Parameter | Type              | Description                                                        |
|----------:|-------------------|--------------------------------------------------------------------|
|    **fs** | _Filesystem type_ | Filesystem                                                         |
|  **path** | `text`            | Path to list. If it is a single file, returns that file            |
|   **max** | `bigint`          | Limit the number of files to be returned. No limit **by default**. |

Returns a set of `omni_vfs_types_v1.file`

!!! warning "Use caution if the directory might contain a lot of files"

    If there are a lot of files, this function will use a lot of memory and will take a long time. To safeguard
    against this, use of `max` parameter is **strongly recommended**.

    One of the reasons why this function has a long name is to force its users to use it carefully and sparingly.

## `omni_vfs.file_info()`

Provides file information (similar to POSIX `stat`)

| Parameter | Type              | Description      |
|----------:|-------------------|------------------|
|    **fs** | _Filesystem type_ | Filesystem       |
|  **path** | `text`            | Path to the file |

Returns a value of the `omni_vfs_types_v1.file_info` type.

If file does not exist, returns `null` as there no information to be retrieved
about it. In all other cases expected to raise an exception.

## `omni_vfs.read()`

Reads a chunk of the file.

|       Parameter | Type              | Description                                                                        |
|----------------:|-------------------|------------------------------------------------------------------------------------|
|          **fs** | _Filesystem type_ | Filesystem                                                                         |
|        **path** | `text`            | Path to the file                                                                   |
| **file_offset** | `bigint`          | Offset to read at. Defaults to `0`.                                                |
|  **chunk_size** | `bigint`          | Number of bytes to read. By default, tries to read to the end [^chunk_size-limit]. |

Returns a `bytea` value

[^chunk_size-limit]: Chunk size is currently limited to 1GB.

## `omni_vfs.write()`

Reads a chunk of the file.

|       Parameter | Type              | Description                                             |
|----------------:|-------------------|---------------------------------------------------------|
|          **fs** | _Filesystem type_ | Filesystem                                              |
|        **path** | `text`            | Path to the file                                        |
|     **content** | `bytea`           | Bytes to write                                          |
| **create_file** | `boolean`         | Create a file if it does not exist. `false` by default. |
|      **append** | `boolean`         | Append file. `false` by default. |

Returns the number of bytes written, as `bigint`.

## Backends

Currently, omni_vfs provides the following backends:

### `omni_vfs.local_fs` (local file system)

This backend can be created by invoking `omni_vfs.local_fs(mount)`, where `mount` is the directory that will be mounted by the backend. No access outside of this directory will be permitted. The function returns `omni_vfs.local_fs` type which only contains an identifier of the instance that references `omni_vfs.local_fs_mounts` table.

!!! tip "Who can access the filesystems?"

    `omni_vfs.local_fs_mounts` has row-level security **enabled**, allowing finer tuning
    of access to mounts and modification of the mounts with policies. Such policies can be used to determine
    under which conditions mounting of new directories is possible, and which mounts can be accessed under given
    conditions.

### `omni_vfs.table_fs` (table backed file system)

This backend can be created by invoking `omni_vfs.table_fs('fs')`, where `fs` is the name
of the filesystem. The function returns `omni_vfs.table_fs` type which only contains an
identifier of the instance that references `omni_vfs.table_fs_filesystems` table.

There are two tables for this backend:

* `omni_vfs.table_fs_files` for storing filesystem id, filename, kind and omni_vfs(`update` queries not allowed).

* `omni_vfs.table_fs_file_data` for storing the data and timestamps of `file` kind files.

#### File operation queries

* The following query creates an entry for sample.txt file in `omni_vfs.table_fs_files` in `fs` table_fs filesystem. There is no need to create parent directories, they will be
created automatically if it doesn't exist.
```postgresql
insert into omni_vfs.table_fs_files (filesystem_id, filename, kind)
values 
((omni_vfs.table_fs('fs')).id, '/dir/sample.txt', 'file');
```

* To associate data with the file created above run the following query. It creates an entry in `omni_vfs.table_fs_file_data`. A utility function named `omni_vfs.table_fs_file_id` is provided to obtain file_id given a table_fs filesystem and filename. Only a single data entry can be associated with a given file.
```postgresql
insert into omni_vfs.table_fs_file_data (file_id, data)
values
(
omni_vfs.table_fs_file_id(omni_vfs.table_fs('fs'), '/dir/sample.txt'),
'hello world'::bytea
);
```

* To update and delete the associated data of a file run the following queries:
```postgresql
update omni_vfs.table_fs_file_data
set
data = 'new data'::bytea
where
file_id = omni_vfs.table_fs_file_id(omni_vfs.table_fs('fs'), '/dir/sample.txt');

delete from omni_vfs.table_fs_file_data
where
file_id = omni_vfs.table_fs_file_id(omni_vfs.table_fs('fs'), '/dir/sample.txt');
```

* To delete the file entry run the following query. It only succeeds if it has no associated data entry.
```postgresql
delete from omni_vfs.table_fs_files
where
id = omni_vfs.table_fs_file_id(omni_vfs.table_fs('fs'), '/dir/sample.txt');
```

The [API](#api) described above works for `omni_vfs.table_fs` files as well. It is recommended to use those to list and read the files to get accurate access timestamp.

!!! tip "Directory listing performance"

    Although `omni_vfs.table_fs` can handle millions of files, it is recommended not to have more than few hundred files in one single directory to ensure optimal listing performance.

### `omni_vfs.remote_fs` (remote file system)

Remote filesystem takes a connection string (just like `dblink` does) and a snippet of SQL that
defines a filesystem remotely:

```postgresql
select omni_vfs.remote_fs('dbname=otherdb host=127.0.0.1', $$omni_vfs.local_fs('/path')$$)
```

All normal VFS operations called over this filesystem are proxied to that remote connection.

!!! tip "Performance considerations"

    At this time, connections are not reused, and every time a call is made, a new connection
    is established.

### Runtime backend dispatch

In a real application, to make it possible to use different backends, one can create a file system "factory" function dependent on the environment they are in. For example, when in development, it can look like this:

```postgresql
create function app_filesystem() returns omni_vfs.local_fs
as
$$
select omni_vfs.local_fs('app')
$$ language sql;
```

And in production, a function with the same name will return a different kind of file system backend.

[^other-file-types]:
Other file types (such as sockets) are not currently considered to be of practical use and will be reported as `file`. This may change in the future.
