create type s3_list_objects_v2 as
(
    bucket             text,
    path               text,
    continuation_token text,
    delimiter          text,
    encoding_type      text,
    fetch_owner        bool,
    max_keys           int8,
    prefix             text,
    start_after        text,
    region             text
);

/*{% include "../src/s3_list_objects_v2.sql" %}*/

/*{% include "../src/aws_request_s3_list_objects_v2.sql" %}*/

create type s3_list_objects_v2_meta as
(
    bucket_name             text,
    prefix                  text,
    is_truncated            bool,
    continuation_token      text,
    next_continuation_token text,
    delimiter               text,
    common_prefixes         text[],
    start_after             text,
    encoding_type           text,
    request_charged         bool
);

/*{% include "../src/aws_execute_s3_list_objects_v2.sql" %}*/