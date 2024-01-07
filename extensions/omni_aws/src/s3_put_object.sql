create function
    s3_put_object(bucket text,
                  path text,
                  payload bytea,
                  content_type text default 'application/octet-stream',
                  region text default null)
    returns s3_put_object
    language sql
    immutable as
$$
select
    row (bucket, path, payload, content_type, region)
$$;