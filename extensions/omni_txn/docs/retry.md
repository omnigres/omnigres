# Transaction Retry

When using serializable transactions, it's often necessary to employ a retry strategy in case of serialization failure.
The algorthms
are fairly typical, so repeating them manually doesn't always make a lot of sense. `omni_txn` provides `retry` procedure
to handle such typical cases.

|        Parameter | Type | Description                                                          |
|-----------------:|------|----------------------------------------------------------------------|
|        **stmts** | text | Statement(s) to execute. Multiple statements separated by semicolon. |
| **max_attempts** | int  | Max number of times to retry. 0 means no retries. 10 by default.     |

## Retry attempt

There is a helper function `omni_txn.current_retry_attempt()` that provides retry attempt during the `retry()` call. 0
stands
for the first run, 1 for the first retry, etc.

## Example

Let's consider the following schema:

```postgresql
create table inventory
(
    id           serial primary key,
    product_name text,
    quantity     int
);
insert into inventory (product_name, quantity)
values ('Widget', 100);
```

Now, if we have these two simultaneous transactions happening, the second one may have committed first:

```postgresql
--- Transaction (1)
begin;

select quantity
from inventory
where product_name = 'Widget';

--- and here (2) will happen
update inventory
set quantity = quantity + 20
where product_name = 'Widget';

commit;
-- ERROR: could not serialize access due to read/write dependencies among transactions

--- Transaction (2)
begin;

update inventory
set quantity = quantity - 10
where product_name = 'Widgert';

commit;
```

If we use `omni_txn.retry`, the failed transaction can be driven to completion:

```postgresql
--- (1)
call omni_txn.retry($$
select quantity from inventory where product_name = 'Widget';
update inventory set quantity = quantity + 20
       where product_name = 'Widget'
$$);
--- (2)
call omni_txn.retry($$
update inventory set quantity = quantity - 10
       where product_name = 'Widgert'
$$);
```