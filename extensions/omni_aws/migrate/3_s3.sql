create type s3_storage_class as enum ('STANDARD', 'REDUCED_REDUNDANCY', 'GLACIER', 'STANDARD_IA', 'ONEZONE_IA' , 'INTELLGIENT_TIERING', 'DEEP_ARCHIVE', 'OUTPOSTS', 'GLACIER_IR', 'SNOW');
create type s3_checksum_algorithm as enum ('CRC32', 'CRC32C', 'SHA1', 'SHA256');
create type s3_owner as
(
    display_name text,
    id           text
);
create type s3_restore_status as
(
    is_restore_in_progress bool,
    restore_expiry_date    timestamp
);

create type s3_endpoint as
(
    url text
);

/*{% include "../src/s3_endpoint.sql" %}*/
/*{% include "../src/aws_s3_endpoint.sql" %}*/
/*{% include "../src/digitalocean_s3_endpoint.sql" %}*/

/*{% include "../src/endpoint_url.sql" %}*/