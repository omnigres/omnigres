## Development

### Using `psql` targets

You can call `psql` on any of the extensions in the tree:

```shell
make psql_<EXTENSION NAME>
```

It'll try to find `postgresql.conf`, `pg_hba.conf` and `.psqlrc` in `extensions/<EXTENSION NAME>`
to use when preparing the database and calling psql. If any are found, they will be used. Locations for these can also
be overriden with `POSTGRESQLCONF`, `PGBHACONF` and `PSQLRC`.

If you want to persist a database between runs, use `PSQLDB` environment variable to point to a directory that should
contain the database.

### Source code formatting

All source code to be contributed should be formatted using clang-format 17 through:

```shell≤
cmake --build build --target fix-format
# or, in the build directory
make fix-format
```

#### CLion

You can also set up CLion to do this automatically for you (it is done this correctly most of the time):

![CLion setting for ClangFormat](docs/clion_clang_format.png)

![CLion setting for auto-save formatting](docs/clion_format_on_save.png)
