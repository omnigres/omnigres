CREATE ROLE test_user INHERIT IN ROLE current_user;
CREATE ROLE test_user1 INHERIT IN ROLE current_user;

SET ROLE test_user;

-- Should use current_user as a default role_name
BEGIN;
WITH listener AS (INSERT INTO omni_httpd.listeners (address, port) VALUES ('127.0.0.1', 9003) RETURNING id),
     handler AS (INSERT INTO omni_httpd.handlers (query) VALUES (
$$SELECT omni_httpd.http_response(body => current_user::text) FROM request$$) RETURNING id)
INSERT INTO omni_httpd.listeners_handlers (listener_id, handler_id) SELECT listener.id, handler.id FROM listener, handler;
DELETE FROM omni_httpd.configuration_reloads;
END;

CALL omni_httpd.wait_for_configuration_reloads(1);

-- Can't update it to an arbitrary name
BEGIN;
UPDATE omni_httpd.handlers SET role_name = 'some_role' WHERE role_name = 'test_user';
DELETE FROM omni_httpd.configuration_reloads;
END;
CALL omni_httpd.wait_for_configuration_reloads(1);

-- Can't update it to a name that is not a current user
BEGIN;
UPDATE omni_httpd.handlers SET role_name = 'test_user1' WHERE role_name = 'test_user';
DELETE FROM omni_httpd.configuration_reloads;
END;
CALL omni_httpd.wait_for_configuration_reloads(1);

-- Can update it to a name that is a current user
SET ROLE test_user1;
BEGIN;
UPDATE omni_httpd.handlers SET role_name = 'test_user1' WHERE role_name = 'test_user';
DELETE FROM omni_httpd.configuration_reloads;
END;
CALL omni_httpd.wait_for_configuration_reloads(1);

-- When changing the query, should always set current user
SET ROLE test_user;
UPDATE omni_httpd.handlers SET query = $$SELECT omni_httpd.http_response(body => current_user::text) FROM request$$
    WHERE role_name = 'test_user1' RETURNING role_name;
-- This will work
BEGIN;
UPDATE omni_httpd.handlers SET query = $$SELECT omni_httpd.http_response(body => current_user::text) FROM request$$,
    role_name = 'test_user'
    WHERE role_name = 'test_user1' RETURNING role_name;

DELETE FROM omni_httpd.configuration_reloads;
END;
CALL omni_httpd.wait_for_configuration_reloads(1);


\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/