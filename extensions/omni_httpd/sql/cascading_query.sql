create table routes
(
    name     text not null,
    query    text not null,
    priority int  not null
);

insert
into
    routes (name, query, priority)
values
    ('test', $$SELECT omni_httpd.http_response(body => 'test') FROM request WHERE request.path = '/test'$$, 1),
    ('ping', $$SELECT omni_httpd.http_response(body => 'pong') FROM request WHERE request.path = '/ping'$$, 1);

\pset format wrapped
\pset columns 80

-- Preview the query
select
    omni_httpd.cascading_query(name, query order by priority desc nulls last)
from
    routes;

\pset format aligned

-- Try it
begin;
with
    listener as (insert into omni_httpd.listeners (address, port) values ('127.0.0.1', 9100) returning id),
    handler as (insert into omni_httpd.handlers (query) select
                                                            omni_httpd.cascading_query(name, query
                                                                                       order by priority desc nulls last)
                                                        from
                                                            routes returning id)
insert
into
    omni_httpd.listeners_handlers (listener_id, handler_id)
select
    listener.id,
    handler.id
from
    listener,
    handler;
delete
from
    omni_httpd.configuration_reloads;
end;

call omni_httpd.wait_for_configuration_reloads(1);

\! curl --retry-connrefused --retry 10  --retry-max-time 10 -w '\n\n' --silent http://localhost:9100/test

\! curl --retry-connrefused --retry 10  --retry-max-time 10 -w '\n\n' --silent http://localhost:9100/ping

-- CTE handling

\pset format wrapped
\pset columns 80

select
    omni_httpd.cascading_query(name, query order by priority desc nulls last)
from
    (values
         ('test',
          $$WITH test AS (SELECT 1 AS val) SELECT omni_httpd.http_response(body => 'test') FROM request, Test WHERE request.path = '/test' and test.val = 1$$,
          1),
         ('ping',
          $$WITH test AS (SELECT 1 AS val) SELECT omni_httpd.http_response(body => 'pong') FROM request, Test WHERE request.path = '/ping' and test.val = 1$$,
          1))
        as routes(name, query, priority);

\pset format aligned