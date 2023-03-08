CREATE TABLE users (
    id integer PRIMARY KEY GENERATED ALWAYS AS IDENTITY,
    handle text,
    name text
);

INSERT INTO users (handle, name) VALUES ('johndoe', 'John');

BEGIN;
WITH listener AS (INSERT INTO omni_httpd.listeners (address, port) VALUES ('127.0.0.1', 9000) RETURNING id),
     handler AS (INSERT INTO omni_httpd.handlers (query) VALUES (
$$
WITH
hello AS
(SELECT omni_httpd.http_response(headers => array[omni_httpd.http_header('content-type', 'text/html')], body => 'Hello, <b>' || users.name || '</b>!')
       FROM request
       INNER JOIN users ON string_to_array(request.path,'/', '') = array[NULL, 'users', users.handle]
),
headers AS
(SELECT omni_httpd.http_response(body => request.headers::text) FROM request WHERE request.path = '/headers'),
echo AS
(SELECT omni_httpd.http_response(body => request.body) FROM request WHERE request.path = '/echo'),
not_found AS
(
SELECT omni_httpd.http_response(status => 404, body => json_build_object('method', request.method, 'path', request.path, 'query_string', request.query_string))
       FROM request)
SELECT * FROM hello
UNION ALL
SELECT * FROM headers WHERE NOT EXISTS (SELECT 1 from hello)
UNION ALL
SELECT * FROM echo WHERE NOT EXISTS (SELECT 1 from headers)
UNION ALL
SELECT * FROM not_found WHERE NOT EXISTS (SELECT 1 from echo)
$$) RETURNING id)
INSERT INTO omni_httpd.listeners_handlers (listener_id, handler_id)
SELECT listener.id, handler.id
FROM listener, handler;
DELETE FROM omni_httpd.configuration_reloads;
END;

CALL omni_httpd.wait_for_configuration_reloads(1);

-- Now, the actual tests

-- FIXME: for the time being, since there's no "request" extension yet, we're shelling out to curl

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -w '\n%{response_code}\nContent-Type: %header{content-type}\n\n' http://localhost:9000/test?q=1

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -w '\n%{response_code}\nContent-Type: %header{content-type}\n\n' -d 'hello world' http://localhost:9000/echo

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -w '\n%{response_code}\nContent-Type: %header{content-type}\n\n' http://localhost:9000/users/johndoe

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -A test-agent http://localhost:9000/headers

-- Try changing configuration

BEGIN;

UPDATE omni_httpd.listeners SET port = 9001 WHERE port = 9000;
WITH listener AS (INSERT INTO omni_httpd.listeners (address, port) VALUES ('127.0.0.1', 9002) RETURNING id),
     handler AS (SELECT ls.handler_id AS id FROM omni_httpd.listeners INNER JOIN omni_httpd.listeners_handlers ls ON ls.listener_id = listeners.id WHERE port = 9001)
INSERT INTO omni_httpd.listeners_handlers (listener_id, handler_id)
 SELECT listener.id, handler.id FROM listener, handler;

DELETE FROM omni_httpd.configuration_reloads;
END;

CALL omni_httpd.wait_for_configuration_reloads(1);


\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -w '\n%{response_code}\nContent-Type: %header{content-type}\n\n' http://localhost:9001/test?q=1

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -w '\n%{response_code}\nContent-Type: %header{content-type}\n\n' http://localhost:9002/test?q=1

\! curl --silent http://localhost:9000/test?q=1 || echo "failed as it should"