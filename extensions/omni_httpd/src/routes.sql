create function routes()
    returns table
            (
                router   regclass,
                matcher  regtype,
                match    jsonb,
                priority int,
                handler  regprocedure,
                type     text
            )
    language plpgsql
as
$$
declare
    rec       record;
    route_rec record;
begin
    for rec in select * from omni_httpd.available_routers
        loop
            router := rec.router_relation;
            matcher := 'omni_httpd.urlpattern';
            for route_rec in execute format('select to_json(t) as row from %1$s t', router)
                loop
                    declare
                        keys text[];
                        row  json := route_rec.row;
                    begin
                        select array_agg(key) into keys from (select * from json_object_keys(row) as t(key)) as t;
                        match := jsonb_strip_nulls((row ->> keys[rec.match_col_idx])::jsonb);
                        handler := row ->> keys[rec.handler_col_idx];
                        with
                            function_details as (select
                                                     unnest(p.proallargtypes) as arg_type,
                                                     unnest(p.proargmodes)    as arg_mode
                                                 from
                                                     pg_proc p
                                                 where
                                                     p.oid = handler)
                        select
                            case
                                when arg_mode is not null then
                                    (case
                                         when arg_mode = 'o' then 'handler'
                                         when arg_mode = 'b' then 'middleware'
                                         else 'unsupported'
                                        end)
                                else 'unsuppported' -- Defaults to IN if proargmodes is NULL
                                end as type
                        into type
                        from
                            function_details fd
                            join pg_type     t on fd.
                                                      arg_type = t.oid
                        where
                            t.typname = 'http_outcome' and
                            t.typnamespace = 'omni_httpd'::regnamespace;
                        type := coalesce(type, 'handler');
                        if rec.priority_col_idx is not null then
                            priority := (row ->> keys[rec.priority_col_idx])::int;
                        else
                            priority := 0;
                        end if;
                        return next;
                    end;
                end loop;
        end loop;
    return;
end;
$$ set search_path = '';