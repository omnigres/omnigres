CREATE ROLE test_user INHERIT IN ROLE current_user;
CREATE ROLE test_user1 INHERIT IN ROLE current_user;

SET ROLE test_user;

-- Should use current_user as a default role_name
INSERT INTO omni_httpd.listeners (listen, query) VALUES (array[row('127.0.0.1', 9003)::omni_httpd.listenaddress], $$SELECT omni_httpd.http_response(body => current_user::text) FROM request$$) RETURNING role_name;

-- Can't update it to an arbitrary name
UPDATE omni_httpd.listeners SET role_name = 'some_role' WHERE role_name = 'test_user';

-- Can't update it to a name that is not a current user
UPDATE omni_httpd.listeners SET role_name = 'test_user1' WHERE role_name = 'test_user';

-- Can update it to a name that is a current user
SET ROLE test_user1;
UPDATE omni_httpd.listeners SET role_name = 'test_user1' WHERE role_name = 'test_user';

-- When changing the query, should always set current user
SET ROLE test_user;
UPDATE omni_httpd.listeners SET query = $$SELECT omni_httpd.http_response(body => current_user::text) FROM request$$
    WHERE role_name = 'test_user1' RETURNING role_name;
  -- This will work
UPDATE omni_httpd.listeners SET query = $$SELECT omni_httpd.http_response(body => current_user::text) FROM request$$,
    role_name = 'test_user'
    WHERE role_name = 'test_user1' RETURNING role_name;

SELECT omni_httpd.reload_configuration();

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/