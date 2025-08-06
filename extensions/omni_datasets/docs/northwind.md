# Northwind

```postgresql
create extension omni_datasets;
create schema northwind;
select omni_datasets.instantiate_northwind(schema => 'northwind');
```

Installs the Northwind sample database. Yes, the 90s classic from MS Access and SQL Server!

The main goal is to provide a standard dataset that all other plugins may and should use
for testing. Plugins should also extend this 90s dataset with sample data relevant to their
functionality. For example, a plugin providing pgvector support would extend this with
vectorized data and AI use cases,

Secondary goal is to provide a small but realistic dataset that is installed by default,
so that new users have something they can see and even try some queries with, without needing
to insert their own data.


## About the sample database

The Northwind sample database appeared in Microsoft Access and later SQL Server
in the 90s. It was recently replaced with a more modern sample data set.

Omnigres uses a PostgreSQL compatible version maintained by Yugabyte: https://github.com/yugabyte/yugabyte-db/tree/master/sample

You can read more and view a diagram of the Northwind database at https://github.com/yugabyte/yugabyte-db/wiki/Northwind-Sample-Database


The Northwind sample database is MIT licensed.
