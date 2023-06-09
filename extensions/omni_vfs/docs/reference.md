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

## `omni_vfs.file` type

Describes a file entry.

|    Field | Type                | Description                                   |
|---------:|---------------------|-----------------------------------------------|
| **name** | `text`              | File name                                     |
| **kind** | `omni_vs.file_kind` | File kind (`file`, `dir`) [^other-file-types] |

[^other-file-types]:
Other file types (such as sockets) are not currently considered to be of practical use and will be reported as `file`. This may change in the future.

## `omni_vs.file_info` type

Describes file meta information.

|           Field | Type        | Description                           |
|----------------:|-------------|---------------------------------------|
|        **size** | `bigint`    | File size                             |
|  **created_at** | `timestamp` | File creation time (if available)     |
| **accessed_at** | `timestamp` | File access time (if available)       |
| **modified_at** | `timestamp` | File modification time (if available) |

## `omni_vfs.list()`

Lists a directory or a single file.

|            Parameter | Type              | Description                                                       |
|---------------------:|-------------------|-------------------------------------------------------------------|
|               **fs** | _Filesystem type_ | Filesystem                                                        |
|             **path** | `text`            | Path to list. If it is a single file, returns that file           |
| **fail_unpermitted** | `bool`            | Raise an error if directory can't be open. `true` **by default**. |

Returns a set of `omni_vfs.file` values.

## `omni_vfs.list_recursively()`

This is a helper function implemented for all backends that lists all files recursively.

| Parameter | Type              | Description                                                        |
|----------:|-------------------|--------------------------------------------------------------------|
|    **fs** | _Filesystem type_ | Filesystem                                                         |
|  **path** | `text`            | Path to list. If it is a single file, returns that file            |
|   **max** | `bigint`          | Limit the number of files to be returned. No limit **by default**. |

Returns a set of `omni_vfs.file`

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

Returns a value of the `omni_vfs.file_info` type.

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

## Backends

Currently, omni_vfs provides the following backends:

### `omni_vfs.local_fs` (local file system)

This backend can be created by invoking `omni_vfs.local_fs(mount)`, where `mount` is the directory that will be mounted by the backend. No access outside of this directory will be permitted. The function returns `omni_vfs.local_fs` type which only contains an identifier of the instance that references `omni_vfs.local_fs_mounts` table.

!!! tip "Who can access the filesystems?"

    `omni_vfs.local_fs_mounts` has row-level security **enabled**, allowing finer tuning
    of access to mounts and modification of the mounts with policies. Such policies can be used to determine
    under which conditions mounting of new directories is possible, and which mounts can be accessed under given
    conditions.

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
