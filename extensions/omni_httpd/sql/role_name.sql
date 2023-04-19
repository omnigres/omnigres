create role test_user inherit in role current_user;
create role test_user1 inherit in role current_user;

create role anotherrole inherit in role current_user;
alter role anotherrole nosuperuser;

create role another_superuser superuser;

set role test_user;

create function reset_role() returns omni_httpd.http_outcome
as
$$
declare
    outcome omni_httpd.http_outcome;
begin
    begin
        reset role;
        select omni_httpd.http_response('ok') into outcome;
    exception
        when others then
            select omni_httpd.http_response('failed resetting') into outcome;
    end;
    return outcome;
end;
$$ language plpgsql;

create function set_role_none() returns omni_httpd.http_outcome
as
$$
declare
    outcome omni_httpd.http_outcome;
begin
    begin
        set role none;
        select omni_httpd.http_response('ok') into outcome;
    exception
        when others then
            select omni_httpd.http_response('failed setting to none') into outcome;
    end;
    return outcome;
end;
$$ language plpgsql;


create function set_superuser_role() returns omni_httpd.http_outcome
as
$$
declare
    outcome omni_httpd.http_outcome;
begin
    begin
        set role another_superuser;
        select omni_httpd.http_response('ok') into outcome;
    exception
        when others then
            select omni_httpd.http_response('failed setting superuser role') into outcome;
    end;
    return outcome;
end;
$$ language plpgsql;


-- Should use current_user as a default role
begin;
with
    listener as (insert into omni_httpd.listeners (address, port) values ('127.0.0.1', 9003) returning id),
    handler as (insert into omni_httpd.handlers (query)
        values
            ($$select
                     (case
                       when request.path = '/' then
                         omni_httpd.http_response(body => current_user::text)
                       when request.path = '/reset-role' then
                         reset_role()
                       when request.path = '/set-role-none' then
                         set_role_none()
                       when request.path = '/superuser-role' then
                         set_superuser_role()
                      end)
                    from request$$) returning id)
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

-- Can't update it to an arbitrary name
begin;
update omni_httpd.handlers
set
    role = 'some_role'::regrole
where
    role = 'test_user'::regrole;
delete
from
    omni_httpd.configuration_reloads;
end;
call omni_httpd.wait_for_configuration_reloads(1);

-- Can update it to a name that is not a current user if it is accessible
begin;
update omni_httpd.handlers
set
    role = 'test_user1'::regrole
where
    role = 'test_user'::regrole;
delete
from
    omni_httpd.configuration_reloads;
end;
call omni_httpd.wait_for_configuration_reloads(1);

-- Can update it to a name that is a current user
set role test_user1;
begin;
update omni_httpd.handlers
set
    role = 'test_user1'::regrole
where
    role = 'test_user'::regrole;
delete
from
    omni_httpd.configuration_reloads;
end;
call omni_httpd.wait_for_configuration_reloads(1);

-- After checking permissions, should not change to the new role
begin;
update omni_httpd.handlers
set
    role = 'anotherrole'::regrole
where
    role = 'test_user1'::regrole;
select current_user;
rollback;

-- Can't update it to a name that is not a current user if it is not accessible
begin;
reset role;
alter role current_user nosuperuser;
update omni_httpd.handlers
set
    role = 'anotherrole'::regrole
where
    role = 'test_user1'::regrole;
rollback;

\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/


begin;
reset role;
update omni_httpd.handlers
set
    role = 'anotherrole'::regrole
where
    role = 'test_user1'::regrole;

delete
from
    omni_httpd.configuration_reloads;

commit;
call omni_httpd.wait_for_configuration_reloads(1);

-- Ensure this role is not a superuser
select
    rolsuper
from
    pg_roles
where
    rolname = 'anotherrole';

-- Check current role
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/
\! echo

--- Ensure it's not possible to reset session to superuser
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/reset-role
\! echo

--- Ensure it's not possible to set session to superuser
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/superuser-role
\! echo

--- Ensure it's not possible to set session to none
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/set-role-none
\! echo

-- Ensure that administrator can reset
alter role anotherrole superuser;
begin;
delete
from
    omni_httpd.configuration_reloads;
select omni_httpd.reload_configuration();
commit;
call omni_httpd.wait_for_configuration_reloads(1);


-- Check current role
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/
\! echo

--- Ensure it's now possible to reset session to superuser
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/reset-role
\! echo

--- Ensure it's now possible to set session to superuser
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/superuser-role
\! echo

--- Ensure it's now possible to set session to none
\! curl --retry-connrefused --retry 10  --retry-max-time 10 --silent http://localhost:9003/set-role-none
\! echo
