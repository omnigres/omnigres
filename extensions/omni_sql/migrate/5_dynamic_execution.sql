CREATE OR REPLACE FUNCTION omni_sql.execute_statement (stmt text, stmt_return_type text DEFAULT 'query')
  RETURNS jsonb
AS $pgsql$
DECLARE
  json_ret jsonb;
  rec record;
  stmt_row_count bigint;
BEGIN
  CASE stmt_return_type
  WHEN 'query' THEN
    BEGIN
      json_ret := '[]'::jsonb;
      FOR rec IN EXECUTE stmt LOOP
        json_ret := json_ret || to_jsonb (rec);
      END LOOP;
      RETURN json_ret;
    EXCEPTION
      WHEN invalid_cursor_definition THEN
        RAISE EXCEPTION 'Statement return type should have been ''command'' instead of ''query''.';
    END;
  WHEN 'command' THEN
    EXECUTE stmt;
    GET DIAGNOSTICS stmt_row_count = ROW_COUNT;
    RETURN to_json(format($ret_message$%s rows affected.$ret_message$, stmt_row_count))::jsonb;
  ELSE
    RAISE EXCEPTION 'Invalid statement return type %. Expecting one of {command, query}', stmt_return_type;
  END CASE;
END;
$pgsql$
LANGUAGE plpgsql;

COMMENT ON FUNCTION omni_sql.execute_statement (text, text) IS $comment$Evaluates arbitrary SQL statements.
Given that some SQL commands are imperative by nature (think CREATE TABLE), you need to pass the kind of expected return, by sending one of {'command', 'query'} as an argument.

A 'query' is a valid command, but the converse is not and will signal an error.
$comment$;
