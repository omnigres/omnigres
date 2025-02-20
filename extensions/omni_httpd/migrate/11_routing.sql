drop procedure handler(int, http_request, inout http_outcome);

create procedure handler(int, http_request, out http_outcome)
    language plpgsql as
$$
begin
    $3 := omni_httpd.http_response(status => 404);
end;
$$;

create type urlpattern as
(
    protocol text,
    username text,
    password text,
    hostname text,
    port     int,
    pathname text,
    search   text,
    hash     text,
    method   omni_http.http_method
);

create function urlpattern(pathname text default null, search text default null, hash text default null,
                           method omni_http.http_method default null,
                           hostname text default null, port int default null,
                           protocol text default null, username text default null,
                           password text default null) returns urlpattern
    language sql
    immutable
as
$$
select row (protocol, username, password, hostname, port, pathname, search, hash, method)::omni_httpd.urlpattern;
$$;

create domain route_priority as int;

create materialized view available_routers as
    select
        c.oid::regclass                                                                as router_relation,
        min(case when a.atttypid = 'omni_httpd.urlpattern'::regtype then a.attnum end) as match_col_idx,
        min(case when a.atttypid = 'regprocedure'::regtype then a.attnum end)          as handler_col_idx,
        min(case when a.atttypid = 'route_priority'::regtype then a.attnum end)        as priority_col_idx
    from
        pg_class          c
        join pg_attribute a
             on a.attrelid = c.oid
    where
        a.attnum > 0       -- Skip system columns
            and
        not a.attisdropped -- Skip dropped columns
    group by
        c.oid
    having
                count(*) filter (where a.atttypid = 'omni_httpd.urlpattern'::regtype) = 1 and
                count(*) filter (where a.atttypid = 'regprocedure'::regtype) = 1 and
                count(*) filter (where a.atttypid = 'route_priority'::regtype) = any (array [0, 1]);

create view default_router as
    select *
    from
        (values
             (urlpattern(),
              'handler(int,omni_httpd.http_request)'::regprocedure,
              (-2147483648)::route_priority)) t
    where
        coalesce(not current_setting('omni_httpd.no_init', true)::bool, true); -- should init

refresh materialized view available_routers;

create or replace function refresh_available_routers() returns event_trigger as
$$
begin
    refresh materialized view omni_httpd.available_routers;
end;
$$ language plpgsql;

create event trigger refresh_available_routers
    on ddl_command_end
    when tag in ('CREATE TABLE', 'CREATE TABLE AS', 'CREATE VIEW', 'DROP TABLE', 'DROP VIEW', 'CREATE MATERIALIZED VIEW', 'DROP MATERIALIZED VIEW')
execute function refresh_available_routers();

/*{% include "../src/routes.sql" %}*/

create view routes as
    select *
    from
        routes()
    order by
        priority desc nulls last;

create table urlpattern_router (
    match urlpattern unique,
    handler regprocedure
);

comment on table urlpattern_router is 'Basic default priority router';

create table router_priority (
   priority route_priority
);

comment on table router_priority is 'Router priority specialization';