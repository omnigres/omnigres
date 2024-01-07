create function
    aws_request(request s3_list_objects_v2,
                access_key_id text,
                secret_access_key text,
                region text default 'us-east-1',
                endpoint s3_endpoint default omni_aws.aws_s3_endpoint())
    returns omni_httpc.http_request
    language plpgsql
    immutable
as
$$
declare
    payload      text                     := '';
    query        text                     := '';
    params       text[];
    ts8601       timestamp with time zone := now();
    endpoint_url text;
    endpoint_uri omni_web.uri;
begin

    -- Alphabetical order of params is important

    if request.continuation_token is not null then
        params := params || ('continuation-token=' || request.continuation_token);
    end if;

    if request.delimiter is not null then
        params := params || ('delimiter=' || request.delimiter);
    end if;

    if request.encoding_type is not null then
        params := params || ('encoding_type=' || request.encoding_type);
    end if;

    if request.fetch_owner is not null and request.fetch_owner then
        params := params || 'fetch-owner=true'::text;
    end if;

    params := params || 'list-type=2'::text;

    if request.max_keys is not null then
        params := params || ('max-keys=' || request.max_keys::text);
    end if;

    if request.prefix is not null then
        params := params || ('prefix=' || request.prefix);
    end if;

    if request.start_after is not null then
        params := params || ('start-after=' || request.start_after);
    end if;

    if array_length(params, 1) is not null and array_length(params, 1) > 0 then
        query := array_to_string(params, '&');
    end if;

    if endpoint is null then
        endpoint := omni_aws.aws_s3_endpoint();
    end if;

    if request.region is not null then
        region := request.region;
    end if;

    if request.bucket is null then
        raise exception 'Bucket can not be null';
    end if;


    request.path := omni_web.uri_encode(request.path);

    endpoint_url :=
            omni_aws.endpoint_url(endpoint, bucket => request.bucket, region => region, path => request.path);
    endpoint_uri := omni_web.text_to_uri(endpoint_url);

    -- here null path is same as root
    request.path := coalesce(endpoint_uri.path, '/');

    if not request.path like '/%' then
        request.path := '/' || request.path;
    end if;

    return omni_httpc.http_request(endpoint_url
                                       || (case when endpoint_uri.path is null then '/' else '' end)
                                       || (case when length(query) > 0 then '?' || query else '' end),
                                   headers => array [
                                       omni_http.http_header(
                                               'X-Amz-Content-Sha256',
                                               encode(digest(payload, 'sha256'), 'hex')),
                                       omni_http.http_header(
                                               'X-Amz-Date',
                                               to_char((ts8601 at time zone 'UTC'),
                                                       'YYYYMMDD"T"HH24MISS"Z"')),
                                       omni_http.http_header(
                                               'Authorization',
                                               'AWS4-HMAC-SHA256 Credential=' || access_key_id || '/'
                                                   || to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD') ||
                                               '/' || region ||
                                               '/s3/aws4_request, SignedHeaders=host;x-amz-content-sha256;x-amz-date, Signature='
                                                   || omni_aws.hash_string_to_sign(
                                                       'AWS4-HMAC-SHA256',
                                                       ts8601,
                                                       region,
                                                       's3',
                                                       omni_aws.hash_canonical_request(
                                                               'GET',
                                                               request.path,
                                                               query,
                                                               array ['host:' ||
                                                                      endpoint_uri.host ||
                                                                      coalesce(':' || endpoint_uri.port, ''),
                                                                   'x-amz-content-sha256:' || encode(digest(payload, 'sha256'), 'hex'),
                                                                       'x-amz-date:' ||
                                                                       to_char((ts8601 at time zone 'UTC'),
                                                                               'YYYYMMDD"T"HH24MISS"Z"')],
                                                               '{"host", "x-amz-content-sha256", "x-amz-date"}',
                                                               encode(digest(payload, 'sha256'), 'hex')
                                                           ),
                                                       secret_access_key
                                                   )
                                           )
                                       ]
        );
end;
$$;