create type s3_put_object as
(
    bucket       text,
    path         text,
    payload      bytea,
    content_type text,
    region       text
);

/*{% include "../src/s3_put_object.sql" %}*/
/*{% include "../src/aws_request_s3_put_object.sql" %}*/
/*{% include "../src/aws_execute_s3_put_object.sql" %}*/