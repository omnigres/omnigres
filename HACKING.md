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