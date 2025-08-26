create function resources(group_version text, resource text, label_selector text default null,
                          field_selector text default null)
    returns setof jsonb
    language plpgsql
as
$resources$
declare
    response jsonb = api('/' ||
                         case when group_version in ('v1') then 'api/v1' else 'apis/' || group_version end || '/' ||
                         resource, label_selector => label_selector, field_selector => field_selector);
begin
    return query select jsonb_array_elements(response -> 'items');
end;
$resources$;
