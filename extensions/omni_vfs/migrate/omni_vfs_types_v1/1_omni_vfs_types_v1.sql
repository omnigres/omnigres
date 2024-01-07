create type file_kind as enum ('dir', 'file');

create type file as
(
    name text,
    kind file_kind
);

create type file_info as
(
    size        bigint,
    created_at  timestamp,
    accessed_at timestamp,
    modified_at timestamp,
    kind        file_kind
);

/*{% include "../../src/is_valid_fs.sql" %}*/