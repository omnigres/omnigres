create function endpoint_url(endpoint s3_endpoint, bucket text, region text, path text default '/') returns text
    immutable
    language sql
as
$$
select
        replace(replace(endpoint.url, '${bucket}', bucket), '${region}', region) ||
        (case when path = '/' then '' else '/' || ltrim(path, '/') end)
$$;