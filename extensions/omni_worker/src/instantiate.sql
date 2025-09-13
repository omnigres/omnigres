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


    perform from pg_roles where rolname = 'omni_worker_sql_user';
    if not found then
        create role omni_worker_sql_user;
    end if;

    create function sql(stmt text, wait_ms int default null, run_as regrole default current_role::regrole) returns bool
        language c as
    'MODULE_PATHNAME';

    revoke execute on function sql from public, current_user;
    grant execute on function sql to omni_worker_sql_user;

    execute format('grant all on schema %I to omni_worker_sql_user', schema);


    perform from pg_roles where rolname = 'omni_worker_sql_autostart_admin';
    if not found then
        create role omni_worker_sql_autostart_admin;
        grant omni_worker_sql_user to omni_worker_sql_autostart_admin;
    end if;

    create table sql_autostart_stmt
    (
        stmt     text    not null,
        label text unique,
        role     regrole not null default current_role::regrole,
        position int     not null
    );
    create function no_label_update() returns trigger
        language plpgsql
    as
    $no_label_update$
    begin
        raise exception 'sql_autostart_stmt labels are immutable';
    end;
    $no_label_update$;

    create trigger prevent_label_update
        before update
        on sql_autostart_stmt
        for each row
        when (old.label is not null and old.label is distinct from new.label)
    execute function no_label_update();

    revoke all on table sql_autostart_stmt from public, current_user;
    grant all on table sql_autostart_stmt to omni_worker_sql_autostart_admin;

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
