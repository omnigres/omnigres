# Query Strings

## Parsing query strings

`parse_query_string` takes a query string:

```sql
select omni_web.parse_query_string('key=value')
```

And returns an array of keys and values:

```psql
 parse_query_string 
--------------------
 {key,value}
(1 row)
```

To retrieve individual parameters, you can use `omni_web.param_get` and `omni_web.param_get_all`:

```sql
select omni_web.param_get(omni_web.parse_query_string('a=1&a=2'), 'a');
param_get 
-----------
 1
(1 row)
select omni_web.param_get_all(omni_web.parse_query_string('a=1&a=2'), 'a');
param_get_all
-----------
 1
 2
(1 row)

```
