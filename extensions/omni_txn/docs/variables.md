# Transaction Variables

`omni_txn` extension provides functionality for storing typed information in a
transactional scope.

This is most useful to maintain information across multiple queries in the
transaction, particularly with RLS (Row Level Security) policies in mind.

## Setting a variable

Within a transaction's context, one can set a named variable with its type
specified through the type of the value:

```postgresql
select omni_txn.set_variable('my_variable', true)
```

This code above sets a boolean-typed variable called `my_variable`. In cases
when the type can't be figured out, type casting comes to the rescue:

```postgresql
select omni_txn.set_variable('text_variable', 'value'::text)
```

The last set variable value and type are set until the end of the current
transaction boundary.

Both value _and_ the type of the variable can be changed by subsequent calls to
`set_variable`

## Getting a variable

In order to get a variable from the transaction's context, one needs to specify
a default value with a type in order to get a value:

```postgresql
select omni_txn.get_variable('my_variable', false)
```

The above will return the value of `my_variable` or `false` if it is not found.

!!! tip "Will the default value be returned if variable is set to `null`?"

    No, if `set_variable` was used to set a `null` value, `get_variable` will 
    return `null`.

If a mismatching type information is passed to `get_variable`,
`get_variable` will raise an error indicating the mismatching types in details.

```postgresql
begin;
select omni_txn.set_variable('var', 1::int);
select omni_txn.get_variable('var', false);
-- ERROR:  type mismatch
-- DETAIL:  expected integer, got boolean
```