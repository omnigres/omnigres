create function
    s3_list_objects_v2(bucket text,
                       path text default '/',
                       continuation_token text default null,
                       delimiter text default null,
                       encoding_type text default null,
                       fetch_owner bool default null,
                       max_keys int8 default null,
                       prefix text default null,
                       start_after text default null,
                       region text default null)
    returns s3_list_objects_v2
    language sql
    immutable as
$$
select
    row (bucket, path, continuation_token, delimiter, encoding_type, fetch_owner, max_keys, prefix, start_after, region)
$$;