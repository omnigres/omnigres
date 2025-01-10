create function register_service(service_name text, info jsonb default 'null'::jsonb) returns void
    language plpgsql
as
$code$
begin
    insert into services (name, info)
    values (service_name, info)
    on conflict (name, postmaster_start_time) do update set info = register_service.info;
end;
$code$;