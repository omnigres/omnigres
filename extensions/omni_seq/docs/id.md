---
title: Distributed IDs
---

<!-- @formatter:off -->
# Sequences and Distributed IDs

You may have learned that one of the current limitations of Postgres logical replication is
that [sequence data is not replicated](https://www.postgresql.org/docs/current/logical-replication-restrictions.html):

!!! quote "From Postgres documentation:"
    
    The data in serial or identity columns backed by sequences will of course be replicated
    as part of the table, but the sequence itself would still show the start value on the subscriber. 
    If the subscriber is used as a read-only database, then this should typically not be a problem. 
    If, however, some kind of switchover or failover to the subscriber database is intended, then the
    sequences would need to be updated to the latest values, either by copying the current data from the publisher 
    (perhaps using pg_dump) or by determining a sufficiently high value from the tables themselves.

This may be an undesirable behavior in some scenarios. Indeed, if there was a switchover, and a replica
was to continue with the insertion of new records, it'll start failing with primary key constraint violation
because sequence counters ("sequence data") has not been replicated.

In this extension, you can work around this limitation by using one of the _distributed_ (or _prefixed_) identifier types.
The core of the idea is that every ID should contain a __node identifier (_prefix_)__ and an __identifier__ itself.

## Types

These types are named using the following pattern: __`omni_seq.id_<TYPE>_<TYPE>`__, where `TYPE` is one
of the following:

* `int16`
* `int32`
* `int64`

## How to Use

One can use it as a default value for a primary key, with an explicitly created sequence:

```postgresql
create sequence seq;
create table t
(
  id omni_seq.id_int64_int64 primary key not null default
     omni_seq.id_int64_int64_nextval(node_id, 'seq') -- (1)
);
```

1. `NODE_ID` is either a number assigned to the current node or a unique system identifier (which can be retrieved
using __`omni_seq.system_identifier()`__[^system_identifier] function)


??? question "Why not `generated always as identity`?"

    The reason why we can't use `generated ... as identity` syntax is that this
    functionality is tied to local counters.

    We also can't use generated columns at all, as "the generation expression can only use immutable functions",
    and `nextval` is volatile as it increments the sequence counter.

[^system_identifier]: 

      An integer contained in the pg_control file providing a reasonably unique database cluster identifier. 
      The function is effectively a simplified version of `SELECT system_identifier FROM pg_control_system()`

## Migration Guide

If you already have a table that you might need to prepare for prefixed identifiers, this guide
will show how it can be done relatively easily.

Let's assume we have a table with an `integer` primary key:

```postgresql
create table my_table (
    id integer primary key generated always as identity
);

insert into my_table select from generate_series(1, 10);
```

Now we want to add a 64-bit[^why-64] prefixed identifier, reusing the existing sequence locally.

[^why-64]: Postgres unique system identifier is a 64-bit integer, see [^system_identifier]

```postgresql
create extension if not exists omni_seq;
begin;

lock table my_table; -- (1)

alter table my_table
    alter column id drop identity if exists;

create sequence my_table_id_seq;

alter table my_table
    alter column id type omni_seq.id_int64_int32 
        using omni_seq.id_int64_int32_make(0, id), -- (2)
    alter column id 
        set default omni_seq.id_int64_int32_nextval(
            omni_seq.system_identifier(), 'my_table_id_seq');
commit;
```

1. Do the migration while locking other clients out.
2. `0` here signifies migrated rows.

When we insert into and query the table again, we'll see this:

```postgresql
psql=# insert into my_table values (default) returning id;
      id           
-----------------------
 7222168279780171472:1 -- (1)
(1 row)
psql=# table my_table;
      id           
-----------------------
 0:1
 0:2
 0:3
 0:4
 0:5
 0:6
 0:7
 0:8
 0:9
 0:10
 7222168279780171472:1
(11 rows)
```

1. The actual number you will see will be different