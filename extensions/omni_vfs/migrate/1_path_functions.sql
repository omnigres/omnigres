create function canonicalize_path(path text, absolute bool default false) returns text
    strict
    language c
as
'MODULE_PATHNAME',
'canonicalize_path_pg' immutable;

create function basename(path text) returns text
    strict
    language c
as
'MODULE_PATHNAME',
'file_basename' immutable;

create function dirname(path text) returns text
    strict
    language c
as
'MODULE_PATHNAME',
'file_dirname' immutable;
