create function service_stop(service_name text) returns service_state
    language plpgsql
as
$code$
declare
    result service_state;
begin
    insert into service_operations (name, operation)
    select service_name, 'stop'
    from from current_services
    where state = 'running';
    select state into result from current_services where name = service_name;
    return result;
end;
$code$;