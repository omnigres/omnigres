create function digitalocean_s3_endpoint() returns s3_endpoint
    immutable
    language sql
as
$$
select row ('https://${bucket}.${region}.digitaloceanspaces.com')::omni_aws.s3_endpoint
$$;