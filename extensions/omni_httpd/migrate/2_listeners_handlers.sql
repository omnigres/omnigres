create table listeners
(
    id             integer primary key generated always as identity,
    address        inet          not null default '127.0.0.1',
    port           port          not null default 80,
    effective_port port          not null default 0,
    protocol       http_protocol not null default 'http'
);

create function check_if_role_accessible_to_current_user(role name) returns boolean
as
$$
declare
    result boolean;
begin
    with
        recursive
        role_members as (select
                             roleid,
                             member
                         from
                             pg_auth_members
                         where
                             roleid = (select oid from pg_roles where rolname = role)

                         union all

                         select
                             am.roleid,
                             am.member
                         from
                             pg_auth_members  am
                             join
                                 role_members rm on am.roleid = rm.member)
    select
        true
    into result
    from
        pg_roles r
    where
        (r.rolname = current_user and r.rolsuper) or
        (r.oid in (select member from role_members)) or
        (r.rolname = role and role = current_user)
    limit 1;
    if not found then
        return false;
    else
        return true;
    end if;
end;

$$ language plpgsql;

create table handlers
(
    id        integer primary key generated always as identity,
    query     text not null,
    role_name name not null default current_user check (check_if_role_accessible_to_current_user(role_name))
);

create function handlers_query_validity_trigger() returns trigger
as
'MODULE_PATHNAME',
'handlers_query_validity_trigger' language c;

create constraint trigger handlers_query_validity_trigger
    after insert or update
    on handlers
    deferrable initially deferred
    for each row
execute function handlers_query_validity_trigger();

create table listeners_handlers
(
    listener_id integer not null references listeners (id),
    handler_id  integer not null references handlers (id)
);
create index listeners_handlers_index on listeners_handlers (listener_id, handler_id);

create table configuration_reloads
(
    id          integer primary key generated always as identity,
    happened_at timestamp not null default now()
);

-- Wait for the number of configuration reloads to be `n` or greater
-- Useful for testing
create procedure wait_for_configuration_reloads(n int) as
$$
declare
    c  int = 0;
    n_ int = n;
begin
    loop
        with
            reloads as (select
                            id
                        from
                            omni_httpd.configuration_reloads
                        order by happened_at asc
                        limit n_)
        delete
        from
            omni_httpd.configuration_reloads
        where
            id in (select id from reloads);
        declare
            rowc int;
        begin
            get diagnostics rowc = row_count;
            n_ = n_ - rowc;
            c = c + rowc;
        end;
        exit when c >= n;
    end loop;
end;
$$ language plpgsql;

create function reload_configuration_trigger() returns trigger
as
'MODULE_PATHNAME',
'reload_configuration'
    language c;

create function reload_configuration() returns bool
as
'MODULE_PATHNAME',
'reload_configuration'
    language c;

create trigger listeners_updated
    after update or delete or insert
    on listeners
execute function reload_configuration_trigger();

create trigger handlers_updated
    after update or delete or insert
    on handlers
execute function reload_configuration_trigger();

create trigger listeners_handlers_updated
    after update or delete or insert
    on listeners_handlers
execute function reload_configuration_trigger();