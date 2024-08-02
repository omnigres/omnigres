# Identity Type

How often do you run into a case like this?

```postgresql
 ```sql
create table users
(
    id serial primary key
);
create table orders
(
    id      serial primary key,
    user_id int not null references users (id)
);

select *
from users
         inner join orders on orders.id = users.id

--- Why is this not getting the right results? \o/
--- ooooh... it should have been `on orders.user_id = users.id`
```

In a simple case, it is usually easy to spot the problem. However, real operational queries tend to get a lot more
complicated, with a lot of visual noise in them, and spotting subtle errors is hard.

`omni_id` solves exacly this problem by introduce custom integer-backed types that are comparable to themselves but not
other types (at least, not without explicit casting).

```postgresql
create table users
(
    id user_id primary key default user_id_nextval()
);
create table orders
(
    id      order_id primary key default order_id_nextval(),
    user_id user_id not null references users (id)
);

select *
from users
         inner join orders on orders.id = users.id

-- ERROR: operator does not exist: order_id = user_id
-- ^^^ this just saved us precious time
```

# Usage

This extension defines a single function `identity_type`. In its most primitive form, it will just take a name of a new
type
and will create a `bigint`-backed type:

```postgresql
create extension omni_id;
-- CREATE EXTENSION
select identity_type('user_id');
-- identity_type 
-- ---------------
-- user_id
```

You can also select a different backing integer type (`smallint`, `int`) and a few sequence-related options.

|            Parameter | Type    | Description                                                                                                    |
|---------------------:|---------|----------------------------------------------------------------------------------------------------------------|
|               *type* | regtype | Backing type. `bigint` by default. `int` and `smallint` permitted, as well as their aliases                    |
|           *sequence* | text    | Sequence name. Equal to `<type>_seq` by default                                                                |
|    *create_sequence* | boolean | Should sequence be created? True by default                                                                    |
|          *increment* | bigint  | Sequence increment. Default set to 1                                                                           |
|           *minvalue* | bigint  | Minimum value a sequence can generate. Default set to 1                                                        |
|           *maxvalue* | bigint  | Maximum value a sequence can generate. Default set to the maximum of the underlying type                       |
|              *cache* | bigint  | Enables sequence numbers to be preallocated and stored in memory for faster access                             |
|              *cycle* | boolean | Wrap around when the maxvalue or minvalue has been reached by an ascending or descending sequence respectively |
|        *constructor* | text    | Name of the constructor function                                                                               |
| *create_constructor* | boolean | Should constructor be created? True by default                                                                 |

`identity_type` will also create helper functions for the sequence: `<type>_nextval()`, `<type>_currval()`
and `<type>_setval(<type>, bool)`

## Constructor

When it is necessary to construct an identity type value, one can use a _constructor_ function like this:

```postgresql
select user_id(1);
```