create function
    s3_create_bucket(bucket text,
                     region text default null)
    returns s3_create_bucket
    language sql
    immutable
as
$$
select
    row (bucket, region)
$$;