CREATE TABLE routes (
   name text NOT NULL,
   query text NOT NULL,
   priority INT NOT NULL
);

INSERT INTO routes (name, query, priority) VALUES
  ('test', $$SELECT omni_httpd.http_response(body => 'test') FROM request WHERE request.path = '/test'$$, 1),
  ('ping', $$SELECT omni_httpd.http_response(body => 'pong') FROM request WHERE request.path = '/ping'$$, 1);

\pset format wrapped
\pset columns 80

SELECT omni_httpd.cascading_query(name, query ORDER BY priority DESC NULLS LAST) FROM routes GROUP BY priority ORDER BY priority DESC;

\pset format aligned

-- CTE handling

\pset format wrapped
\pset columns 80

SELECT omni_httpd.cascading_query(name, query ORDER by priority DESC NULLS LAST) FROM (
  VALUES
  ('test', $$WITH test AS (SELECT 1 AS val) SELECT omni_httpd.http_response(body => 'test') FROM request, Test WHERE request.path = '/test' and test.val = 1$$, 1),
  ('ping', $$WITH test AS (SELECT 1 AS val) SELECT omni_httpd.http_response(body => 'pong') FROM request, Test WHERE request.path = '/ping' and test.val = 1$$, 1))
  AS routes(name, query, priority) GROUP BY priority ORDER BY priority DESC;

 \pset format aligned