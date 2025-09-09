create function instantiate(schema regnamespace default 'omni_worker') returns void
    language plpgsql
as
$$
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);


    create table handlers
    (
        library text not null,
        name    text not null,
        primary key (library, name)
    );

    insert
    into
        handlers
    values
        ('MODULE_PATHNAME', 'sql'),
        ('MODULE_PATHNAME', 'timer');

    create function reload_handlers() returns trigger
        language c as
    'MODULE_PATHNAME';

    create trigger reload_handlers
        after insert or update or truncate or delete
        on handlers
        for each statement
    execute function reload_handlers();

    create function sql(stmt text, wait_ms int default null) returns bool
        language c as
    'MODULE_PATHNAME';

    create table sql_autostart_stmt
    (
        stmt     text not null,
        position int  not null
    );

    perform from pg_roles where rolname = 'omni_worker_timer_user';
    if not found then
        create role omni_worker_timer_user;
    end if;

    create function timer_after(delay_ms bigint, fun regproc,
                                run_as regrole default current_role::regrole) returns bigint
        language c as
    'MODULE_PATHNAME';

    create function timer_cancel(timer_id bigint) returns bool
        language c as
    'MODULE_PATHNAME';

    revoke execute on function timer_after, timer_cancel from public, current_user;
    grant execute on function timer_after, timer_cancel to omni_worker_timer_user;
    execute format('grant all on schema %I to omni_worker_timer_user', schema);


end;
$$;
