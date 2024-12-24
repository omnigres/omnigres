create function service_provisioning_trigger() returns trigger
    language plpgsql
as
$code$
begin
    delete from services where postmaster_start_time != pg_postmaster_start_time();
    return new;
end;
$code$;