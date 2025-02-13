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


create materialized view available_routers as
    select
        c.oid::regclass                                                                as router_relation,
        min(case when a.atttypid = 'omni_httpd.urlpattern'::regtype then a.attnum end) as match_col_idx,
        min(case when a.atttypid = 'regprocedure'::regtype then a.attnum end)          as handler_col_idx
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
                count(*) filter (where a.atttypid = 'regprocedure'::regtype) = 1;


create view default_router as
    select *
    from (values (urlpattern(),
                  'handler(int,omni_httpd.http_request,omni_httpd.http_outcome)'::regprocedure)) t
    where  coalesce(not current_setting('omni_httpd.no_init', true)::bool, true); -- should init

refresh materialized view available_routers;

/*{% include "../src/routes.sql" %}*/

create view routes as
select *
from routes();