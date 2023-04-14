---
title: Prefixed Sequences
---

<!-- @formatter:off -->
# Prefixed Sequences

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

In this extension, you can work around this limitation by using one of the _prefixed sequence_ types.

## Types

These types are named using the following pattern: __`omni_seq.prefix_seq_<TYPE>_<TYPE>`__, where `TYPE` is one
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
  id omni_seq.prefix_seq_int64_int64 primary key not null default
     omni_seq.prefix_seq_int64_int64_nextval(NODE_ID, 'seq') -- (1)
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