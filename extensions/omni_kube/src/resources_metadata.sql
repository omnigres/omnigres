create function resources_metadata(group_version text, resource text)
    returns jsonb
    language plpgsql
as
$resources$
declare
    response jsonb = api('/' ||
                         case when group_version in ('v1') then 'api/v1' else 'apis/' || group_version end || '/' ||
                         resource);
begin
    return response -> 'metadata';
end;
$resources$;
