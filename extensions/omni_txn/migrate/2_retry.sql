create procedure retry(stmts text, max_attempts int default 10)
    language c as
'MODULE_PATHNAME';

comment on procedure retry is $$
Retry serializable transaction on statements `stmts`, `max_attempt` number of times (10 by default)
$$;

create function current_retry_attempt()
    returns int
    language c as
'MODULE_PATHNAME';

comment on function current_retry_attempt() is $$
Within `omni_txn.retry` indicates the current retry attempt. Zero during the first run.
$$;