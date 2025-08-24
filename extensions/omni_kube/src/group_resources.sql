create function group_resources(group_version text)
    returns table
            (
                name                 text,
                singular_name        text,
                namespaced           boolean,
                kind                 text,
                verbs                text[],
                storage_version_hash text
            )
    language plpgsql
as
$group_resources$
declare
    response jsonb = api('/' ||
                         case when group_version in ('v1') then 'api/v1' else 'apis/' || group_version end);
begin
    return query select data ->> 'name',
                        data ->> 'singularName',
                        (data -> 'namespaced')::boolean,
                        data ->> 'kind',
                        array(select jsonb_array_elements_text(data -> 'verbs')),
                        data ->> 'storageVersionHash'
                 from (select jsonb_array_elements(response -> 'resources') data) resources;
end;
$group_resources$;
