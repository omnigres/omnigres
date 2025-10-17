create table if not exists motd -- (1)
(
    id        int primary key generated always as identity,
    content   text,
    posted_at timestamp default now()
);

-- (2)
create or replace function show_motd() returns setof omni_httpd.http_outcome as
$$
select
    omni_httpd.http_response('Posted at ' || posted_at || E'\n' || content)
from
    motd
order by
    posted_at desc
limit 1;
$$ language sql;

-- (3)
create or replace function no_motd() returns setof omni_httpd.http_outcome as
$$
select omni_httpd.http_response('No MOTD');
$$
    language sql;

-- (4)
create or replace function update_motd(request omni_httpd.http_request) returns omni_httpd.http_outcome as
$$
insert
into
    motd (content)
values
    (convert_from(request.body, 'UTF8'))
returning omni_httpd.http_response(status => 201);
$$
    language sql;

-- (5)
update omni_httpd.handlers
set
    query = (select
                 -- (6)
                 omni_httpd.cascading_query(name, query order by priority desc nulls last)
             from
                 (values
                      ('show', $$select show_motd() from request where request.method = 'GET'$$, 1),
                      ('update', $$select update_motd(request.*) from request where request.method = 'POST'$$, 1),
                      ('fallback', $$select no_motd() from request where request.method = 'GET'$$,
                       0)) handlers(name, query, priority));