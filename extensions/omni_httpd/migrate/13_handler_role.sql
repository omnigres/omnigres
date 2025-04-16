alter event trigger refresh_available_routers disable;
drop materialized view available_routers cascade;

create materialized view available_routers as
select c.oid::regclass                                                                as router_relation,
       min(case when a.atttypid = 'omni_httpd.urlpattern'::regtype then a.attnum end) as match_col_idx,
       min(case when a.atttypid = 'regprocedure'::regtype then a.attnum end)          as handler_col_idx,
       min(case when a.atttypid = 'route_priority'::regtype then a.attnum end)        as priority_col_idx,
       min(case when a.atttypid = 'regprocedure'::regtype then a.attname end)::name   as handler_col_name
from pg_class c
         join pg_attribute a
              on a.attrelid = c.oid
where a.attnum > 0       -- Skip system columns
  and not a.attisdropped -- Skip dropped columns
group by c.oid
having count(*) filter (where a.atttypid = 'omni_httpd.urlpattern'::regtype) = 1
   and count(*) filter (where a.atttypid = 'regprocedure'::regtype) = 1
   and count(*) filter (where a.atttypid = 'route_priority'::regtype) = any (array [0, 1]);

refresh materialized view available_routers;

alter event trigger refresh_available_routers enable;

create table handler_role
(
    handler regprocedure not null primary key,
    role    regrole      not null
);
