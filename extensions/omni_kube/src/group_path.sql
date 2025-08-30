create function group_path(group_version text) returns text
    immutable
return '/' || case when group_version in ('v1') then 'api/v1' else 'apis/' || group_version end;
