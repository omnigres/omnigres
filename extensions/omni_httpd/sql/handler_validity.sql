-- Invalid queries
INSERT INTO omni_httpd.handlers (query) VALUES($$SELECT * FROM no_such_table$$);
INSERT INTO omni_httpd.handlers (query) VALUES($$SELECT request.pth FROM request$$);
INSERT INTO omni_httpd.handlers (query) VALUES($$$$);
INSERT INTO omni_httpd.handlers (query) VALUES($$SELECT; SELECT$$);

-- Valid query at the end of the transaction
BEGIN;
INSERT INTO omni_httpd.handlers (query) VALUES($$SELECT * FROM no_such_table$$);
CREATE TABLE no_such_table ();
END;

DROP TABLE no_such_table;
