# Table Mapping

This extension allows you to map tables to JSON, extending `to_jsonb` 
function by overloading it for a particular table type.

By default, one can use `to_jsonb` on any existing table type and get a JSON 
out:

```postgresql
 create table example (
                          id           integer primary key generated always as identity,
                          first_name   text,
                          last_name    text,
                          dob          date
 );

select to_jsonb(example.*) from example;
```

results in

```json
{
  "id": 1, 
  "dob": "1971-12-12", 
  "last_name": "Doe", 
  "first_name": "John"
}
```

However, the moment you need to perform some transformation, it becomes less 
useful. This is where this extension helps. One can define a mapping for a 
table using `omni_json.define_table_mapping(type, annotation)` where 
annotation is a JSON document:

```postgresql
select omni_json.define_table_mapping(example, '{}')
```

## Configuring columns

By specifying `columns` object with a for a given column, one can configure 
column properties.

```postgresql
select omni_json.define_table_mapping(example, $$
{
  "columns": { "my_column": { ... } }
}
$$);
```


### Renaming keys

`path` annotation for a column can be used to rename it

```json
{
  "columns": {
    "dob": {
      "path": "date_of_birth"
    }
  }
}
```

Now, if you re-run `to_jsonb` you will get this:

```json
{
  "id": 1, 
  "date_of_birth": "1971-12-12", 
  "last_name": "Doe", 
  "first_name": "John"
}
```

### Moving keys

`path` annotation can also be used to move columns to a given path when given 
an array of keys:

```json
{
  "columns": {
    "first_name": {
      "path": ["name", "first"]
    },
    "last_name": {
      "path": ["name", "last"]
    }
  }
}
```

Now, if you re-run `to_jsonb` you will get this:

```json
{
  "id": 1, 
  "date_of_birth": "1971-12-12",
  "name": {
    "last": "Doe",
    "first": "John"
  }
}
```

In fact, it can also move columns to arrays. 

```json
{
  "columns": {
    "first_name": {
      "path": ["name", 0]
    },
    "last_name": {
      "path": ["name", 1]
    }
  }
}
```

Now, if you re-run `to_jsonb` you will get this:

```json
{
  "id": 1, 
  "date_of_birth": "1971-12-12",
  "name": [
    "John",
    "Doe"
  ]
}
```

### Column exclusion

Imagine we don't want to show date of birth in the above example. We can do 
so by simply excluding it:

```json
{
  "columns": {
    "dob": {
      "exclude": true
    }
  }
}
```

Now, if you re-run `to_jsonb` you will get this:

```json
{
  "id": 1,
  "last_name": "Doe",
  "first_name": "John"
}
```

## Operational Guide

### Retrieving JSON

You can use `to_jsonb(table_name.*)` as you would typically do, but the
transformation rules described above will apply.

```postgresql
select
    to_jsonb(products.*)
from
    products
```

### Updating from JSON

You can update explicitly listed fields using the following construct:

```postgresql
update people
set
    -- `dob`, `first_name` and `last_name` are allowed to be updated
    (dob, first_name, last_name) =
        (select
             dob,
             first_name,
             last_name
         from
             jsonb_populate_record(people.*,
                                   '{"dob": "1981-12-12"}'))
where
    id = some_id 
```

### Inserting JSON

Similarly to update, JSON can be also inserted

```postgresql
insert
into
    people (dob, first_name, last_name)
    (select
         dob,
         first_name,
         last_name
     from
         jsonb_populate_record(null::people,
                               '{"first_name": "Jane", "last_name": "Doe", "dob": "1981-12-12"}'))
```