-- Correctly drop the old procedure
DROP PROCEDURE IF EXISTS omni_txn.retry(text, integer, boolean, boolean, record, boolean);

-- Create the new version as a PROCEDURE with the timeout parameter
CREATE PROCEDURE omni_txn.retry(
    stmts text,
    max_attempts integer DEFAULT 10,
    repeatable_read boolean DEFAULT false,
    collect_backoff_values boolean DEFAULT false,
    params record DEFAULT NULL,
    linearize boolean DEFAULT false,
    timeout_ms integer DEFAULT 0
)
LANGUAGE C
AS 'MODULE_PATHNAME', 'retry';