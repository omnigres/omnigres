CREATE TABLE users (
    id integer PRIMARY KEY GENERATED ALWAYS AS IDENTITY,
    handle text,
    name text
);

INSERT INTO users (handle, name) VALUES ('johndoe', 'John');

BEGIN;
WITH handler AS (INSERT INTO omni_httpd.handlers VALUES (DEFAULT) RETURNING id),
     listener AS (INSERT INTO omni_httpd.listeners (address, port, handler_id) VALUES ('127.0.0.1', 9000, (SELECT id FROM handler)) RETURNING id),
     queries AS (INSERT INTO omni_httpd.handlers_queries (priority, name, query, handler_id) VALUES
      (1, 'hello',
      $$SELECT omni_httpd.http_response(headers => array[omni_httpd.http_header('content-type', 'text/html')], body => 'Hello, <b>' || users.name || '</b>!')
       FROM request
       INNER JOIN users ON string_to_array(request.path,'/', '') = array[NULL, 'users', users.handle]
      $$, (SELECT id FROM handler)),
      (1, 'headers',
      $$SELECT omni_httpd.http_response(body => request.headers::text) FROM request WHERE request.path = '/headers'$$, (SELECT id FROM handler)),
      (1, 'echo',
      $$SELECT omni_httpd.http_response(body => request.body) FROM request WHERE request.path = '/echo'$$, (SELECT id FROM handler)),
      (0, 'not_found',
      $$SELECT omni_httpd.http_response(status => 404, body => json_build_object('method', request.method, 'path', request.path, 'query_string', request.query_string))
       FROM request$$,  (SELECT id FROM handler)) RETURNING id)
SELECT * FROM listener, queries LIMIT 0;
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
INSERT INTO omni_httpd.listeners (address, port, handler_id) VALUES ('127.0.0.1', 9002, (SELECT handler_id AS id FROM omni_httpd.listeners WHERE port = 9001));

DELETE FROM omni_httpd.configuration_reloads;
END;

CALL omni_httpd.wait_for_configuration_reloads(1);


\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -w '\n%{response_code}\nContent-Type: %header{content-type}\n\n' http://localhost:9001/test?q=1

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent -w '\n%{response_code}\nContent-Type: %header{content-type}\n\n' http://localhost:9002/test?q=1

\! curl --silent http://localhost:9000/test?q=1 || echo "failed as it should"