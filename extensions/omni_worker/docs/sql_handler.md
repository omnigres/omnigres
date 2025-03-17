# SQL handler

`omni_worker` includes a built-in handler for SQL statement execution. It's configured to include handle
SQL messages by default. If you don't want it to be availablel, remove the corresponding entry from the `handlers`
table.

The most simple execution form is the one where you pass the SQL statement and let it execute:

```postgresql
select omni_worker.sql('sql-stmt')
```

This form does not wait for the result of the execution, and immediately returns `null` upon scheduling the work
to be executed.

## Executing with timeout

If you need to wait until the execution has occurred, specify waiting time in milliseconds:

```postgresql
select omni_worker.sql('sql-stmt', wait_ms => 1000)
```

This will wait for 1000 milliseconds (1 second). If the execution has been completed,
it'll return `true` if it was successful or `false` if not. If the execution hasn't been completed, it'll return `null`.




