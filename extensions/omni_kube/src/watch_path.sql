create function watch_path(path text, resource_version text default null)
    returns text
    immutable
return
    path || case
                when position('?' in path) > 0 then '&'
                else '?' end || 'watch=1' || coalesce('&resourceVersion=' || resource_version, '');
