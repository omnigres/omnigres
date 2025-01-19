# Embedded SQLite

`omni_sqlite` is an extension that adds the capability to use SQLite
databases as a first-class data type within Postgres.

The extension can be installed into a Postgres in the normal way:

```postgresql
create extension omni_sqlite;
```

!!! tip "`omni_sqlite` is a templated extension"

    `omni_sqlite` is a templated extension. This means that by installing it, its default-instantiated
     into `omni_sqlite` but can be instantiated into any other schema:

     ```postgresql 
     select omni_sqlite.instantiate([schema => 'omni_sqlite'])
     ```

### Key Advantages

#### Multitenancy

This extension simplifies multitenancy by embedding isolated SQLite
databases directly into Postgres. Each SQLite database is entirely
independent of its surrounding Postgres environment, eliminating
the need for complex RLS policies, permissions, alternate database in
a cluster, or other traditional multitenancy mechanisms. This allows
user data to be securely stored in separate rows without additional
configuration.

#### Client-Server Synchronization

Another powerful use case is being able to exchange and synchronize 
such databases between peers. For example, you can seamlessly exchange
data between client and server using either SQLite’s native SQL text format
or the standard binary format via `sqlite_serialize()` and `sqlite_deserialize()`.

## Creating SQLite Databases


There are three main objects in the extension, the `sqlite` type and
the `sqlite_exec()` and `sqlite_query()` functions.  New databases can
be created by casting an initialization string to the `sqlite` type:

```postgresql
select 'create table user_config (key text, value text)'::omni_sqlite.sqlite;
sqlite                      
--------------------------------------------------
PRAGMA foreign_keys=OFF;                        +
BEGIN TRANSACTION;                              +
CREATE TABLE user_config (key text, value text);+
COMMIT;                                         +
(1 row)
```

Note that while you are *seeing* the SQL text representation of the
sqlite object, it is stored internally in Postgres as a compact binary
representation of an in-memory SQLite database.

## Modifying SQLite Objects

This new sqlite instance can now be inserted into a table and
manipulated with the `sqlite_exec()` function, for example:

```postgresql
create table customer (
    id bigserial primary key,
    name text not null,
    data omni_sqlite.sqlite default 'create table user_config (key text, value text);'
);

insert into customer (name) values ('bob') returning *;
 id | name |                       data                       
----+------+--------------------------------------------------
  1 | bob  | PRAGMA foreign_keys=OFF;                        +
    |      | BEGIN TRANSACTION;                              +
    |      | CREATE TABLE user_config (key text, value text);+
    |      | COMMIT;                                         +
    |      | 
(1 row)

update customer
    set data = omni_sqlite.sqlite_exec(data, $$INSERT INTO user_config VALUES ('color', 'blue')$$)
    returning *;
 id | name |                                 data                                 
----+------+----------------------------------------------------------------------
  1 | bob  | PRAGMA foreign_keys=OFF;                                            +
    |      | BEGIN TRANSACTION;                                                  +
    |      | CREATE TABLE user_config (key text, value text);                    +
    |      | INSERT INTO user_config(rowid,"key",value) VALUES(1,'color','blue');+
    |      | COMMIT;                                                             +
    |      | 
(1 row)
```

Notice how the default value for the `data` column in the new table
will initialize a new SQLite database into that column.  All new rows
in `customer` will contain an SQLite database named `data`
which in turn contains a table `user_config`.

The `sqlite_exec(db, query)` function takes a SQLite database and a
query as an argument, executes that query and returns the same
database, so this can be used for chaining updates to the same
database through multiple calls.

## Querying SQLite Objects

The `sqlite_query(db, query)` function is a Set Returning Function
(SRF) that returns `setof record` with a row for each SQlite
result row from the given sqlite query.  Using the standard 'as'
syntax these values can be mapped to table like Postgres results:

```postgresql
select * from omni_sqlite.sqlite_query(
        (select data from customer),
        'SELECT rowid, key, value from user_config')
    as (id integer, key text, value text);
 id |  key  | value 
----+-------+-------
  1 | color | blue
(1 row)
```

!!! question "What types are supported?"

    This extension currently support integers, floats and text.

## Serialize/Deserialize

There is support for serializing and deserializing SQLite
databases as `bytea` byte array types.

```postgresql
select pg_typeof(omni_sqlite.sqlite_serialize('create table foo (x)'));
 pg_typeof 
-----------
 bytea
(1 row)
```

`sqlite_deserialize` takes a `bytea` representation of the database
and expands it into sqlite object:

```postgresql
select omni_sqlite.sqlite_deserialize(omni_sqlite.sqlite_serialize('create table foo (x)'));
 sqlite_deserialize    
--------------------------
PRAGMA foreign_keys=OFF;+
BEGIN TRANSACTION;      +
CREATE TABLE foo (x);   +
COMMIT;                 +

(1 row)
```