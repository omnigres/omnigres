# omni_schema

This extension allows application schemas to be managed easily, both during development and deployment.

## Migrations

This extension provides `migrate_from_fs` function that executes SQL migrations from a file system (provided by the `omni_vfs` extension.)

```postgresql
select *
from
    omni_schema.migrate_from_fs(omni_vfs.local_fs('/path/to/project/migrations'))
```

It returns a set of `text` with each element being the file name executed.

The above invocation is most useful for development environment or deployment that is done with the backing of a local file system. In the near future, more file systems will be added, and that will facilitate more ergonomic scenarios.

!!! tip "Can't define a new filesystem?"

    You can specify `path` optional parameter to indicate where the migrations reside
    on the file system:

    ```postgresql
    select *
    from
      omni_schema.migrate_from_fs(omni_vfs.local_fs('/path/to/project'), 'migrations')
    ```

This function will recursively find all files with `.sql` extension and apply them ordered by path name, excluding those that were already applied before. For this purpose, it maintains the `omni_schema.migrations` table.

|         Column | Type      | Description                               |
|---------------:|-----------|-------------------------------------------|
|         **id** | int       | Unique identifier                         |
|       **name** | text      | Migration (file) name                     |
|  **migration** | text      | The source code of the migration          |
| **applied_at** | timestamp | Time of migration application [^grouping] |

[^grouping]: The timestamp defaults to `now()` which means that migrations applied in the same transaction all get the same value of `applied_at`, which can be used for grouping them together.

## Object reloading

For certain types of schema objects , it is possible to reload their contents without having to create a migration every time they change (which is fairly suboptimal, especially when it comes to tracking their changes in a version control system.)
The types supported are:

* **functions**
* **policies**
* **views**

This extension provides `load_from_fs` function that will reload all such objects from a local on a file system (provided by the `omni_vfs` extension), similar to `migrate_from_fs`:

```postgresql
select *
from
    omni_schema.load_from_fs(omni_vfs.local_fs('/path/to/project/migrations'))
```

Its return type and parameters are currently identical to those of `migrate_from_fs`.