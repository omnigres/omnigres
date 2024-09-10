# Function Signature Types

Function signature types are essentially a way to type functions with a specific arguments and return type signature.
Whereas `regproc` and `regprocedure` denote that the type is a function or a procedure, the type itself does not convey
its signature. This limits their usefulness.

Function signature types, on the other hand, can be used to call functions that match the signature.

## Defining a function signature type

To define such a type, use `omni_types.function_signature_type(name, ...signature)`:

```postgresql
select omni_types.function_signature_type('sig', 'text', 'int')
```

In the above example, it will define type `sig` that denotes a function that takes `text` and returns `int` (the last
element in the variadic `signature` argument is the return type)

| *Parameter* | *Type*           | *Description*                                            |
|------------:|------------------|----------------------------------------------------------|
|      *name* | name             | Name of the newly created type                           |
| *signature* | variadic regtype | Signature of the function. Last value is the return type |

Calling the function with the same arguments will return the same type without attempting to recreate it, if the
signatures match. Otherwise, an error will occur.

### Defining a signature from a prototype

One can also define a function signature type by using an existing function as a prototype
with `omni_types.function_signature_type(name, function)`:

```postgresql
select omni_types.function_signature_type_of('sig', 'length(text)')
```

| *Parameter* | *Type* | *Description*                                                                  |
|------------:|--------|--------------------------------------------------------------------------------|
|      *name* | name   | Name of the newly created type                                                 |
|      *func* | text   | Name (`pg_backend_pid`) or argument signature (`length(text)`) of the function |

## Casting to a function signature type

With the type defined, one can cast a function name into it. It will return a `sig` type successfully if it matches:

```postgresql
select 'length'::sig
```

If no function is matching the signature captured by the type, this will fail with an error. If this is undesirable,
`<TYPE>_conforming_function(text)` can be used instead:

```postgresql
with funcs as (select sig_confirming_function(name::text) as f from names)
select funcs
where f is not null;
```

## Calling a function

Function signature types can be called (and it is one of its primary benefits!) by using an auto-generated `call_<NAME>`
function:

```postgresql
select call_sig('length', 'hello')
-- returns 5 as this is what `length(text)` would return
```