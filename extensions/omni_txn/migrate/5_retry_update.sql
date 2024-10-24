drop procedure if exists retry(stmts text, max_attempts int, repeatable_read boolean,
                               collect_backoff_values boolean, params record);


create procedure retry(
    stmts text, 
    max_attempts int default 10, 
    repeatable_read boolean default false, 
    collect_backoff_values boolean default false, 
    timeout interval default null,          -- Adding timeout parameter or other changes
    params record default null::record
)
    language c as
'MODULE_PATHNAME';


comment on procedure retry is $$
Retry serializable transaction on statements `stmts`, retrying `max_attempt` number of times (10 by default).
`collect_backoff_values` controls whether the backoff values used for sleeping will be recorded for debugging/testing purposes.
You can also specify a `timeout` to control the total time the retry attempts will take.
$$;
