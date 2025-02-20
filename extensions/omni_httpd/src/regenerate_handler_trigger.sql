create function regenerate_handler_trigger() returns trigger
    security definer
    language plpgsql as
$$
declare
    rec      record;
    body     text;
    proc_oid oid;
    different_roles bool;
    _role_name      name;
begin
    -- Safety check
    select 'omni_httpd.handler'::regproc::oid into proc_oid;

    select distinct on (role_name) count(*) > 1, role_name
    from omni_httpd.handlers
    group by role_name
    into different_roles, _role_name;

    if different_roles then
        for rec in select listener_id, handler_id, query, role_name
                   from omni_httpd.listeners_handlers
                            inner join omni_httpd.handlers on handlers.id = listeners_handlers.handler_id
            loop
                declare
                    ident text;
                begin
                    ident := 'handler_' || rec.listener_id || '_' || rec.handler_id;
                    execute format(
                            'create or replace function omni_httpd.%I(omni_httpd.http_request) returns omni_httpd.http_outcome language plpgsql security definer as $__sql__$ begin return (with request as (select ($1).*) %s limit 1); end;$__sql__$',
                            ident, rec.query);
                    execute format('alter function omni_httpd.%I(omni_httpd.http_request) owner to %s', ident,
                                   rec.role_name::regrole);
                end;
            end loop;
    end if;

    body :=
            format('create or replace procedure omni_httpd.handler(int, omni_httpd.http_request, out omni_httpd.http_outcome) language plpgsql as $___sql___$ begin $3 := (');

    if not different_roles then
    body := body || 'with request as (select ($2).*) select * from (';
    end if;

    select body || omni_httpd.cascading_query('handler_' || listener_id || '_' || handler_id,
                                              case
                                                  when different_roles then
                                                      'select * from omni_httpd.handler_' || listener_id || '_' ||
                                                      handler_id ||
                                                      '($2) where $1 = ' || listener_id
                                                  else 'select * from (' || query || ') q where $1 = ' || listener_id end
                   )
    from omni_httpd.listeners_handlers
             inner join omni_httpd.handlers on handlers.id = listeners_handlers.handler_id
    into body;

    if not different_roles then
        body := body || ') t';
    end if;

    body := body || format(' limit 1); end; $___sql___$ ');

    -- Enact the regeneration
    execute body;

    if not different_roles then
        execute format(
                'alter procedure omni_httpd.handler(int, omni_httpd.http_request, out omni_httpd.http_outcome) owner to %s',
                       _role_name::regrole);
        execute format(
                'alter procedure omni_httpd.handler(int, omni_httpd.http_request, out omni_httpd.http_outcome) security definer',
                       _role_name::regrole);
        execute format(
                'alter procedure omni_httpd.handler(int, omni_httpd.http_request, out omni_httpd.http_outcome) owner to %s',
                       _role_name::regrole);
    end if;

    -- Perform safety check
    perform where proc_oid = 'omni_httpd.handler'::regproc::oid;
    if not found then
        raise exception 'omni_httpd.handler changed OID, invariant violation';
    end if;
    return new;
end;
$$;