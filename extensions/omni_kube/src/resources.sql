create function resources(group_version text, resource text)
    returns setof jsonb
    language plpgsql
as
$resources$
declare
    response jsonb = api('/' ||
                         case when group_version in ('v1') then 'api/v1' else 'apis/' || group_version end || '/' ||
                         resource);
begin
    return query select jsonb_array_elements(response -> 'items');
end;
$resources$;
