create procedure retry(stmts text, max_attempts int default 10, repeatable_read boolean default false,
                       collect_backoff_values boolean default false ,timeout interval default '0 seconds' )
    language c as
'MODULE_PATHNAME';

comment on procedure retry is $$
Retry serializable transaction on statements `stmts`, `max_attempt` number of times (10 by default). 
`collect_backoff_values` controls if the backoff values used for sleeping will be recorded for debugging/testing purposes (false in order to increase performance).
`timeout` specifies the maximum time in seconds to attempt the transaction.
$$;

create function current_retry_attempt()
    returns int
    language c as
'MODULE_PATHNAME';

comment on function current_retry_attempt() is $$
Within `omni_txn.retry` indicates the current retry attempt. Zero during the first run.
$$;

create function retry_backoff_values()
    returns table
            (
              backoff_sleep_time int
            )
    volatile
    language c
as
'MODULE_PATHNAME';

comment on function retry_backoff_values () is $$
Retrieves the backoff values used in the `omni_txn.retry` run. Empty in the first run.
$$;
