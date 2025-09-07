create function path_with(path text, label_selector text default null, field_selector text default null,
                          timeout_seconds int default null, dry_run boolean default null)
    returns text
    immutable
return
    path ||
    (case
         when position('?' in path) > 0 then '&'
         else '?' end) ||
    concat_ws('&',
              case when dry_run then 'dryRun=All' end,
              'labelSelector=' || omni_web.url_encode(label_selector),
              'fieldSelector=' || omni_web.url_encode(field_selector),
              'timeoutSeconds=' || timeout_seconds
    );
