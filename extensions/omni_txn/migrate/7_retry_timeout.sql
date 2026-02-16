drop procedure retry(stmts text, max_attempts int, repeatable_read boolean,
                     collect_backoff_values boolean, params record, linearize boolean);

create procedure retry(stmts text, max_attempts int default 10, repeatable_read boolean default false,
                       collect_backoff_values boolean default false,
                       params record default null::record, linearize boolean default false,
                       timeout_ms int default null)
    language c as
'MODULE_PATHNAME';

comment on procedure retry is $$
Retry serializable transaction on statements `stmts`, `max_attempt` number of times (10 by default).
`collect_backoff_values` controls if the backoff values used for sleeping will be recorded for
debugging/testing purposes (false in order to increase performance). `timeout_ms` controls the
maximum time allowed for retries before aborting.
$$;
