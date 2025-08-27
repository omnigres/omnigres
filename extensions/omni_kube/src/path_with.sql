create function path_with(path text, label_selector text default null, field_selector text default null,
                          timeout_seconds int default null)
    returns text
    immutable
return
    path ||
    (case
         when label_selector is null and field_selector is null then ''
         when position('?' in path) > 0 then '&'
         else '?' end) ||
    coalesce('labelSelector=' || omni_web.url_encode(label_selector), '') ||
    coalesce('&fieldSelector=' ||
             omni_web.url_encode(field_selector),
             '') || coalesce('&timeoutSeconds=' || timeout_seconds, '');
