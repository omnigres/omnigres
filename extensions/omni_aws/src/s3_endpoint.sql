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