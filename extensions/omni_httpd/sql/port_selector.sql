--- Force omni_httpd to select a port
BEGIN;
WITH listener AS (INSERT INTO omni_httpd.listeners (address, port, handler_id) VALUES ('127.0.0.1', 0, (SELECT id FROM handler)) RETURNING id),
     handler AS (INSERT INTO omni_httpd.handlers VALUES (DEFAULT) RETURNING id)
     SELECT * FROM listener;
DELETE FROM omni_httpd.configuration_reloads;
END;

CALL omni_httpd.wait_for_configuration_reloads(1);

-- Ensure port was updated
SELECT count(*) FROM omni_httpd.listeners WHERE port = 0;

-- TODO: Test request on a given port. Needs non-shell http client