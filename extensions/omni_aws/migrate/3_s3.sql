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

create function s3_endpoint(url text, path_style boolean default true) returns s3_endpoint
    immutable
    language sql
as
$$
select
    case
        when path_style then row (rtrim(url, '/') || '/${bucket}')::omni_aws.s3_endpoint
        else row (url)::omni_aws.s3_endpoint end
$$;

create function aws_s3_endpoint() returns s3_endpoint
    immutable
    language sql
as
$$
select row ('https://${bucket}.s3.${region}.amazonaws.com')::omni_aws.s3_endpoint
$$;

create function digitalocean_s3_endpoint() returns s3_endpoint
    immutable
    language sql
as
$$
select row ('https://${bucket}.${region}.digitaloceanspaces.com')::omni_aws.s3_endpoint
$$;

create function endpoint_url(endpoint s3_endpoint, bucket text, region text, path text default '/') returns text
    immutable
    language sql
as
$$
select
        replace(replace(endpoint.url, '${bucket}', bucket), '${region}', region) ||
        (case when path = '/' then '' else '/' || ltrim(path, '/') end)
$$;