DROP FUNCTION omni_txn.retry(text, integer, boolean, boolean, record, boolean);

CREATE FUNCTION omni_txn.retry(
    stmts text,
    max_attempts integer DEFAULT 10,
    repeatable_read boolean DEFAULT false,
    collect_backoff_values boolean DEFAULT false,
    params record DEFAULT NULL,
    linearize boolean DEFAULT false,
    timeout_ms integer DEFAULT 0
) RETURNS void
    LANGUAGE C
AS
'MODULE_PATHNAME',
'retry';