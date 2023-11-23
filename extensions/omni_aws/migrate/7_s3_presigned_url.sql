create function
    s3_presigned_url(bucket text,
                     path text,
                     access_key_id text,
                     secret_access_key text,
                     expires int default 604800, -- 7 days
                     region text default 'us-east-1',
                     endpoint s3_endpoint default omni_aws.aws_s3_endpoint()
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
    endpoint_url text;
    endpoint_uri omni_web.uri;
begin

    path := omni_web.uri_encode(path);

    endpoint_url :=
            omni_aws.endpoint_url(endpoint, bucket => bucket, region => region, path => path);
    endpoint_uri := omni_web.text_to_uri(endpoint_url);

    path := endpoint_uri.path;

    if not path like '/%' then
        path := '/' || path;
    end if;

    raise notice '%', path;
    raise notice '%', endpoint_url;

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
                        'host:' || endpoint_uri.host || coalesce(':' || endpoint_uri.port, '')
                        ],
                    '{"host"}',
                    'UNSIGNED-PAYLOAD'
                ),
            secret_access_key
        );

    return endpoint_url ||
           '?X-Amz-Algorithm=AWS4-HMAC-SHA256' ||
           '&X-Amz-Credential=' || omni_web.url_encode(credential) ||
           '&X-Amz-Date=' || amz_date ||
           '&X-Amz-Expires=' || expires ||
           '&X-Amz-SignedHeaders=host' ||
           '&X-Amz-Signature=' || auth;
end
$$;
