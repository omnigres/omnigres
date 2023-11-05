create function
    s3_presigned_url(bucket text,
                     path text,
                     access_key_id text,
                     secret_access_key text,
                     expires int default 604800, -- 7 days
                     region text default 'us-east-1',
                     endpoint_url text default null
) returns text
    language plpgsql
    immutable
as
$$
declare
    amz_date   text;
    auth       text;
    credential text;
    ts8601     timestamp
                   with
                       time zone := now();
begin

    if not path like '/%' then
        path := '/' || path;
    end if;


    if endpoint_url is null then
        endpoint_url := omni_aws.s3_endpoint_url(bucket, region);
    else
        path := '/' || bucket || path;
    end if;

    credential := access_key_id || '/'
                      || to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD') ||
                  '/' || region ||
                  '/s3/aws4_request';
    amz_date := to_char((ts8601 at time zone 'UTC'),
                        'YYYYMMDD"T"HH24MISS"Z"');
    auth := omni_aws.hash_string_to_sign(
            'AWS4-HMAC-SHA256',
            ts8601,
            region,
            's3',
            omni_aws.hash_canonical_request(
                    'GET',
                    path,
                    'X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=' || omni_web.url_encode(credential) ||
                    '&X-Amz-Date=' || amz_date || '&X-Amz-Expires=' || expires || '&X-Amz-SignedHeaders=host',
                    array [
                            'host:' || substring(endpoint_url from '^[a-zA-Z]+://(.*)')
                        ],
                    '{"host"}',
                    'UNSIGNED-PAYLOAD'
                ),
            secret_access_key
        );

    return endpoint_url || path ||
           '?X-Amz-Algorithm=AWS4-HMAC-SHA256' ||
           '&X-Amz-Credential=' || omni_web.url_encode(credential) ||
           '&X-Amz-Date=' || amz_date ||
           '&X-Amz-Expires=' || expires ||
           '&X-Amz-SignedHeaders=host' ||
           '&X-Amz-Signature=' || auth;
end
$$;
