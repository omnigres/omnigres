create function resources_path(group_version text, resource text, namespace text default null) returns text
    immutable
return group_path(group_version) || '/' || coalesce('namespaces/' || namespace || '/', '') ||
       resource;
