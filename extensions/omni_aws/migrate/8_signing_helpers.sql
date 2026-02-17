-- AWS Signing Helpers
-- Extracts common patterns from aws_request functions to improve readability and maintainability.

-- Helper: format timestamp to AWS date format (YYYYMMDDTHHMMSSZ)
create function aws_amz_date(ts timestamp with time zone)
    returns text
    language sql
    immutable
as
$$
select to_char((ts at time zone 'UTC'), 'YYYYMMDD"T"HH24MISS"Z"')
$$;

-- Helper: format timestamp to date-only (YYYYMMDD)
create function aws_date_stamp(ts timestamp with time zone)
    returns text
    language sql
    immutable
as
$$
select to_char((ts at time zone 'UTC'), 'YYYYMMDD')
$$;

-- Helper: build credential string (e.g. AKID/20260217/us-east-1/s3/aws4_request)
create function aws_credential(access_key_id text, ts timestamp with time zone, region text, service text)
    returns text
    language sql
    immutable
as
$$
select access_key_id || '/' || omni_aws.aws_date_stamp(ts) || '/' || region || '/' || service || '/aws4_request'
$$;

-- Helper: compute SHA-256 hex hash of payload (bytea)
create function aws_payload_hash(payload bytea)
    returns text
    language sql
    immutable
as
$$
select encode(digest(payload, 'sha256'), 'hex')
$$;

-- Helper: compute SHA-256 hex hash of payload (text overload)
create function aws_payload_hash(payload text)
    returns text
    language sql
    immutable
as
$$
select encode(digest(payload, 'sha256'), 'hex')
$$;

-- Helper: extract host header value from URI (host:port or just host)
create function aws_host_header(uri omni_web.uri)
    returns text
    language sql
    immutable
as
$$
select uri.host || coalesce(':' || uri.port, '')
$$;

-- Helper: build full Authorization header value
create function aws_authorization_header(
    access_key_id text,
    secret_access_key text,
    ts timestamp with time zone,
    region text,
    service text,
    signed_headers_str text,
    canonical_request_hash text)
    returns text
    language sql
    immutable
as
$$
select 'AWS4-HMAC-SHA256 Credential=' || omni_aws.aws_credential(access_key_id, ts, region, service)
           || ', SignedHeaders=' || signed_headers_str
           || ', Signature=' || omni_aws.hash_string_to_sign(
               'AWS4-HMAC-SHA256', ts, region, service, canonical_request_hash, secret_access_key)
$$;

-- Now replace existing functions to use the helpers

create or replace function
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
    payload_hash text                     := omni_aws.aws_payload_hash('');
    query        text                     := '';
    params       text[];
    ts8601       timestamp with time zone := now();
    endpoint_url text;
    endpoint_uri omni_web.uri;
    amz_date     text;
    host_value   text;
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

    amz_date := omni_aws.aws_amz_date(ts8601);
    host_value := omni_aws.aws_host_header(endpoint_uri);

    return omni_httpc.http_request(endpoint_url
                                       || (case when endpoint_uri.path is null then '/' else '' end)
                                       || (case when length(query) > 0 then '?' || query else '' end),
                                   headers => array [
                                       omni_http.http_header('X-Amz-Content-Sha256', payload_hash),
                                       omni_http.http_header('X-Amz-Date', amz_date),
                                       omni_http.http_header('Authorization',
                                               omni_aws.aws_authorization_header(
                                                       access_key_id, secret_access_key, ts8601, region, 's3',
                                                       'host;x-amz-content-sha256;x-amz-date',
                                                       omni_aws.hash_canonical_request(
                                                               'GET', request.path, query,
                                                               array [
                                                                   'host:' || host_value,
                                                                   'x-amz-content-sha256:' || payload_hash,
                                                                   'x-amz-date:' || amz_date],
                                                               '{"host", "x-amz-content-sha256", "x-amz-date"}',
                                                               payload_hash)))
                                       ]
        );
end;
$$;

create or replace function aws_request(request s3_put_object,
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
    ts8601       timestamp with time zone := now();
    endpoint_url text;
    endpoint_uri omni_web.uri;
    payload_hash text;
    amz_date     text;
    host_value   text;
begin

    if request.region is not null then
        region := request.region;
    end if;

    request.path := omni_web.uri_encode(request.path);

    endpoint_url :=
            omni_aws.endpoint_url(endpoint, bucket => request.bucket, region => region, path => request.path);
    endpoint_uri := omni_web.text_to_uri(endpoint_url);

    request.path := endpoint_uri.path;

    if not request.path like '/%' then
        request.path := '/' || request.path;
    end if;

    payload_hash := omni_aws.aws_payload_hash(request.payload);
    amz_date := omni_aws.aws_amz_date(ts8601);
    host_value := omni_aws.aws_host_header(endpoint_uri);

    return omni_httpc.http_request(endpoint_url
                                       || (case when endpoint_uri.path is null then '/' else '' end),
                                   method => 'PUT',
                                   body => request.payload,
                                   headers => array [
                                       omni_http.http_header('Content-Type', request.content_type),
                                       omni_http.http_header('X-Amz-Content-Sha256', payload_hash),
                                       omni_http.http_header('X-Amz-Date', amz_date),
                                       omni_http.http_header('Authorization',
                                               omni_aws.aws_authorization_header(
                                                       access_key_id, secret_access_key, ts8601, region, 's3',
                                                       'content-type;host;x-amz-content-sha256;x-amz-date',
                                                       omni_aws.hash_canonical_request(
                                                               'PUT', request.path, '',
                                                               array [
                                                                   'content-type:' || request.content_type,
                                                                   'host:' || host_value,
                                                                   'x-amz-content-sha256:' || payload_hash,
                                                                   'x-amz-date:' || amz_date],
                                                               '{"content-type", "host", "x-amz-content-sha256", "x-amz-date"}',
                                                               payload_hash)))
                                       ]
        );
end;
$$;

create or replace function aws_request(request s3_create_bucket,
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
    ts8601       timestamp with time zone := now();
    path         text                     := '/';
    payload      bytea                    := convert_to('', 'utf-8');
    endpoint_url text;
    endpoint_uri omni_web.uri;
    payload_hash text;
    amz_date     text;
    host_value   text;
begin

    if request.region is not null then
        region := request.region;
    end if;

    endpoint_url :=
            omni_aws.endpoint_url(endpoint, bucket => request.bucket, region => region);
    endpoint_uri := omni_web.text_to_uri(endpoint_url);

    -- here null path is same as root
    path := coalesce(endpoint_uri.path, '/');

    if not path like '/%' then
        path := '/' || request.bucket;
    end if;

    payload_hash := omni_aws.aws_payload_hash(payload);
    amz_date := omni_aws.aws_amz_date(ts8601);
    host_value := omni_aws.aws_host_header(endpoint_uri);

    return omni_httpc.http_request(endpoint_url,
                                   method => 'PUT',
                                   body => payload,
                                   headers => array [
                                       omni_http.http_header('Content-Type', 'application/xml'),
                                       omni_http.http_header('X-Amz-Content-Sha256', payload_hash),
                                       omni_http.http_header('X-Amz-Date', amz_date),
                                       omni_http.http_header('Authorization',
                                               omni_aws.aws_authorization_header(
                                                       access_key_id, secret_access_key, ts8601, region, 's3',
                                                       'content-type;host;x-amz-content-sha256;x-amz-date',
                                                       omni_aws.hash_canonical_request(
                                                               'PUT', path, '',
                                                               array [
                                                                   'content-type:application/xml',
                                                                   'host:' || host_value,
                                                                   'x-amz-content-sha256:' || payload_hash,
                                                                   'x-amz-date:' || amz_date],
                                                               '{"content-type","host", "x-amz-content-sha256", "x-amz-date"}',
                                                               payload_hash)))
                                       ]
        );
end;
$$;

create or replace function
    s3_presigned_url(bucket text,
                     path text,
                     access_key_id text,
                     secret_access_key text,
                     expires int default 604800, -- 7 days
                     region text default 'us-east-1',
                     endpoint s3_endpoint default omni_aws.aws_s3_endpoint(),
                     method omni_http.http_method default 'GET'
) returns text
    language plpgsql
    immutable
as
$$
declare
    amz_date     text;
    credential   text;
    ts8601       timestamp with time zone := now();
    endpoint_url text;
    endpoint_uri omni_web.uri;
    signature    text;
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

    credential := omni_aws.aws_credential(access_key_id, ts8601, region, 's3');
    amz_date := omni_aws.aws_amz_date(ts8601);
    signature := omni_aws.hash_string_to_sign(
            'AWS4-HMAC-SHA256',
            ts8601,
            region,
            's3',
            omni_aws.hash_canonical_request(
                    method::text,
                    path,
                    'X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=' || omni_web.url_encode(credential) ||
                    '&X-Amz-Date=' || amz_date || '&X-Amz-Expires=' || expires || '&X-Amz-SignedHeaders=host',
                    array [
                        'host:' || omni_aws.aws_host_header(endpoint_uri)
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
           '&X-Amz-Signature=' || signature;
end
$$;
