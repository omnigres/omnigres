-- Invalid queries
BEGIN;
INSERT INTO omni_httpd.handlers VALUES (DEFAULT) RETURNING id AS handler_id \gset
INSERT INTO omni_httpd.handler_queries (handler_id, query) VALUES(:handler_id, $$SELECT * FROM no_such_table$$);
INSERT INTO omni_httpd.handler_queries (handler_id, query) VALUES(:handler_id, $$SELECT request.pth FROM request$$);
INSERT INTO omni_httpd.handler_queries (handler_id, query) VALUES(:handler_id, $$$$);
INSERT INTO omni_httpd.handler_queries (handler_id, query) VALUES(:handler_id, $$SELECT; SELECT$$);
ROLLBACK;

-- Valid query at the end of the transaction
BEGIN;
INSERT INTO omni_httpd.handlers VALUES (DEFAULT) RETURNING id AS handler_id \gset
INSERT INTO omni_httpd.handlers_queries (handler_id, query) VALUES(:handler_id, $$SELECT * FROM no_such_table$$);
CREATE TABLE no_such_table ();
END;

DROP TABLE no_such_table;
