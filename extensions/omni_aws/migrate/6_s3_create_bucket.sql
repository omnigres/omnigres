create type s3_create_bucket as
(
    bucket text,
    region text
);

/*{% include "../src/s3_create_bucket.sql" %}*/
/*{% include "../src/aws_request_s3_create_bucket.sql" %}*/
/*{% include "../src/aws_execute_s3_create_bucket.sql" %}*/
