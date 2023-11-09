create function s3_endpoint_url(bucket text, region text default 'us-east-1') returns text
    language sql
    immutable as
$$
select 'https://' || bucket || '.s3.' || region || '.amazonaws.com'
$$;

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
