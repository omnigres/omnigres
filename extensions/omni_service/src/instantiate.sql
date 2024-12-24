create function instantiate(schema regnamespace default 'omni_service') returns void
    language plpgsql
as
$$
begin
    perform set_config('search_path', schema::text, true);

    create table services
    (
        name                  text        not null,
        info                  jsonb       not null default 'null':: jsonb,
        postmaster_start_time timestamptz not null default pg_postmaster_start_time(),
        primary key (name, postmaster_start_time)
    );
    /*{% include "register_service.sql" %}*/
    execute format('alter function register_service set search_path to %I', schema);

    create sequence service_regtype_seq;

    /*{% include "service_provisioning_trigger.sql" %}*/
    execute format('alter function service_provisioning_trigger() set search_path to %I', schema);

    create trigger service_provisioning_trigger
        before insert
        on services
        for each row
    execute function service_provisioning_trigger();

    create type service_operation as enum ('start', 'stop');

    create table service_operations
    (
        name                  text              not null,
        postmaster_start_time timestamptz       not null default pg_postmaster_start_time(),
        operation             service_operation not null,
        applied_at            timestamptz       not null default statement_timestamp(),
        foreign key (name, postmaster_start_time) references services (name, postmaster_start_time)
    );

    create type service_state as enum ('not-running', 'running', 'stopped');

    /*{% include "service_start.sql" %}*/
    execute format('alter function service_start set search_path to %I', schema);

    /*{% include "service_stop.sql" %}*/
    execute format('alter function service_stop set search_path to %I', schema);

    create view current_services as
    select s.name,
           info,
           (case
                when so.operation is null then 'not-running'
                when so.operation = 'start' then 'running'
                when so.operation = 'stop' then 'stopped'
               end)::service_state as state
    from services s
             left join (select distinct on (name, postmaster_start_time) *
                        from service_operations
                        order by name, postmaster_start_time, applied_at desc) so
                       on so.name = s.name and so.postmaster_start_time = s.postmaster_start_time
    where s.postmaster_start_time = pg_postmaster_start_time();

end;
$$;