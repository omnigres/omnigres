create table listeners
(
    id             integer primary key generated always as identity,
    address        inet          not null default '127.0.0.1',
    port           port          not null default 80,
    effective_port port          not null default 0,
    protocol       http_protocol not null default 'http'
);

/*{% include "../src/check_if_role_accessible_to_current_user.sql" %}*/

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

/*{% include "../src/wait_for_configuration_reloads.sql" %}*/

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