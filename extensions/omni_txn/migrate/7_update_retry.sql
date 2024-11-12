drop procedure retry(stmts text, max_attempts int, repeatable_read boolean,
                               collect_backoff_values boolean, params record);


create procedure retry(
    stmts text, 
    max_attempts int default 10, 
    repeatable_read boolean default false, 
    collect_backoff_values boolean default false, 
    params record default null::record,    
    timeout interval default null           
)
    language c as
'MODULE_PATHNAME';


comment on procedure retry is $$
Retry serializable transaction on statements `stmts`, retrying up to `max_attempts` times or until `timeout` is reached,
whichever occurs first. If both parameters are omitted, the procedure defaults to retrying 10 times with no time limit.

Scenarios:
1. If both `timeout` and `max_attempts` are provided, retries continue until the earlier of the two conditions.
2. If only `timeout` is specified, retries continue until the timeout is reached.
3. If only `max_attempts` is specified, retries continue up to that number of attempts.
4. If neither is specified, the default behavior is to retry 10 times without a time limit.

If a timeout is exceeded during a single attempt, retries are immediately aborted.
$$;
