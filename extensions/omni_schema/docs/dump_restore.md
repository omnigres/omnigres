# Database Dump and Restore Functions

The `omni_schema` extension provides two functions for dumping and restoring Postgres databases across connections.

## dump()

Dumps schema and/or data from a remote Postgres database using `pg_dump` command line tool.

```sql
select omni_schema.dump(connstr, schema, data);
```

### Parameters

| Parameter | Type   | Default    | Description                                    |
|-----------|--------|------------|------------------------------------------------|
| `connstr` | `text` | (required) | libpq connection string to the source database |
| `schema`  | `bool` | `true`     | Include schema (DDL) in the dump               |
| `data`    | `bool` | `true`     | Include data (DML) in the dump                 |

### Return Value

Returns `text` containing the SQL dump output.

### Examples

```sql
-- Dump everything (schema + data)
select omni_schema.dump('host=remote.db.com dbname=mydb user=postgres');

-- Schema only
select omni_schema.dump('host=localhost dbname=test', true, false);

-- Data only  
select omni_schema.dump('host=localhost dbname=test', false, true);
```

## restore()

Restores SQL schema to a remote Postgres database using `psql` command-line tool.

```sql
select omni_schema.restore(connstr, schema);
```

### Parameters

| Parameter | Type   | Description                                        |
|-----------|--------|----------------------------------------------------|
| `connstr` | `text` | libpq connection string to the target database     |
| `schema`  | `text` | SQL statements to execute (obtained from `dump()`) |

### Return Value

Returns `void`.

### Example

```sql
-- Restore a previously dumped schema
do
$$
    declare
      dumped_sql text;
    begin
      dumped_sql := omni_schema.dump('host=source.db user=postgres dbname=mydb');
      perform omni_schema.restore('host=target.db user=postgres dbname=newdb', dumped_sql);
    end
$$;
```

## Security Considerations

Functions that call `pg_dump` and `psql` ar marked `SECURITY DEFINER`, meaning they execute with the privileges of the
function creator (`omni_schema_external_tool_caller` role created by the extension) rather
than the caller. This allows us to contain privileges required to do such operations.

!!! warning "Authentication caveat"

    It is important to note that actual invocations of `pg_dump` and `psql` will assume the credentials of a     local user, which may result in slightly different authentication. We do, however, validate connectivity with the given connection string prior to these invocations. To avoid unwanted local authentication, we recommend setting `host` connection string parameter to a network address.

### Protection Against SQL Injection

- **Connection string validation**: Both functions use `dblink_connect()` to validate connection strings before passing
  them to shell commands
- **Shell escaping**: Connection strings are properly escaped using `%L` format specifier to prevent shell injection

### Access Control

These functions can:

- Execute `pg_dump` and `psql` commands on the system
- Connect to any database the Postgres server can reach

### Connection String Security

Connection strings may contain sensitive information (passwords, hostnames). Ensure appropriate logging and audit
controls are in place.
