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

This extension provides `load_from_fs` function that will reload all such
objects from a local file system (provided by the `omni_vfs` extension),
similar to `migrate_from_fs`:

```postgresql
select *
from
  omni_schema.load_from_fs(omni_vfs.local_fs('/path/to/project/migrations'))
```

Its return type and parameters are currently identical to those
of `migrate_from_fs`.

!!! tip "Ignoring files"

    In order to avoid loading particular files that match a language or a tool
    filename pattern, one can put `omni_schema[[ignore]]` somewhere inside such
    file (for example, in a comment) and `omni_schema` will not load it.

## Multi-language functions

Object reloading functionality allows one to load functions from '.sql' files
which can contain SQL or PL/pgSQL functions defined verbatim:

```postgresql title='test_function.sql'
create function test_function(a integer) returns integer
  language sql
as
$$
select a > 1
$$;

create function test_function(a integer) returns integer
  language plpgsql
as
$$
begin
  return a > 1;
end
$$;
```

Such files can contain multiple function definitions.

One can note, however, that SQL or PL/pgSQL is not always the best fit for a
particular problem. This is reflected in the fact that Postgres has an ecosystem
of other programming languages. However, writing code in those languages inside
of SQL files is a sub-par development experience: syntax highlighting,
auto-completion may not work; external tools that work with this languages are
unaware of this embedding technique.

To address those challenges, we have created language specific extension such as [omni_python](../../../extensions/omni_python/docs/intro.md) that provides first-class support for Python. `load_from_fs` also installs required python libraries that are mentioned in requirements.txt file.

If you would like to see support for other languages, please create an [issue](https://github.com/omnigres/omnigres/issues) or let us know in our [discord](https://discord.gg/ezRUnnnc2A)

* `omni_schema.languages` tables that defines mapping of languages

Currently supported languages:

* SQL and PL/pgSQL (`.sql`) [^sql-languages-table]
* Python (`.py`)
* Perl (`.pl`, `.trusted.pl`)
* Tcl (`.tcl`, `.trusted.tcl`)
* Rust (`.rs`)

If an extension required for the support of a language is not installed, files
in that language will be skipped and a notice will be given, similar to this
one:

```
Extension pltclu required for language pltclu (required for foo.tcl)
is not installed
```

The support for languages is configurable through `omni_schema.languages` table:

|             Column | Type | Description                                                                            |
|-------------------:|------|----------------------------------------------------------------------------------------|
| **file_extension** | text | Filename extension without the preceding <br/>dot. _Examples: `py`, `trusted.pl`, `rs` |
|       **language** | name | Language identifier to be used `create function ... language`                          |
|      **extension** | name | Extension that implements the language, if required                                    |

[^sql-languages-table]: SQL language is always supported, even if corresponding
entry is removed from `omni_schema.languages`. This behavior may change in the
future.


`omni_schema.auxiliary_tools` - A congifuration table for installing modules (or packages):

|                                Column | Type | Description                                                                               |
|--------------------------------------:|---------|----------------------------------------------------------------------------------------|
| **filename_stem**                     | text    | Filename without the extension (eg. `requirements`)                                    |
| **filename_extension**                | text    | Only the extension part of the filename (eg. `txt`)                                    |
| **processor**                         | name    | Name of the procedure                                                                  |
| **processor_extension**               | name    | Extension that implements the language, if required                                    |
| **priority**                          | integer | Priority at which to process the files. Related to `omni_schema.languages`             |

!!! warning "Don't modify the records in the table"
    The records in the table are used by internal APIs.
    Most of the times an end user don't need to directly modify it.