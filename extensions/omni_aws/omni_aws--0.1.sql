create function hash_canonical_request(method text,
                                       uri text,
                                       query text,
                                       headers text[],
                                       signed_headers text[],
                                       payload_hash text)
    returns text
    language sql
    immutable
as
$$
select
    encode(digest(omni_aws.canonical_request(method, uri, query, headers, signed_headers, payload_hash), 'sha256'),
           'hex')
$$;

create function canonical_request(method text,
                                  uri text,
                                  query text,
                                  headers text[],
                                  signed_headers text[],
                                  payload_hash text)
    returns text
    language sql
    immutable
as
$$
select
                                                method || chr(10) || uri || chr(10) || query || chr(10) ||
                                                array_to_string(headers, chr(10)) || chr(10) || chr(10) ||
                                                array_to_string(signed_headers, ';') || chr(10) || payload_hash
$$;


create function hash_string_to_sign(
    algorithm text,
    ts8601 timestamp with time zone,
    region text,
    service text,
    canonical_request_hash text,
    secret_access_key text)
    returns text
    language sql
    immutable
as
$$
select
    encode(hmac((((((((algorithm || chr(10) ||
                       to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD"T"HH24MISS"Z"') || chr(10) ||
                       to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD')) || '/' || region) || '/') ||
                    service) || '/aws4_request') || chr(10)) || canonical_request_hash)::bytea, hmac('aws4_request',
                                                                                                     hmac(
                                                                                                             service::bytea,
                                                                                                             hmac(
                                                                                                                     region::bytea,
                                                                                                                     hmac(
                                                                                                                             to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD'),
                                                                                                                             ('AWS4' || secret_access_key),
                                                                                                                             'sha256'),
                                                                                                                     'sha256'),
                                                                                                             'sha256'),
                                                                                                     'sha256'),
                'sha256'), 'hex')
$$;

create function s3_endpoint_url(bucket text, region text default 'us-east-1') returns text
    language sql
    immutable as
$$
select 'https://' || bucket || '.s3.' || region || '.amazonaws.com'
$$;

create type s3_list_objects_v2 as
(
    bucket             text,
    path               text,
    continuation_token text,
    delimiter          text,
    encoding_type      text,
    fetch_owner        bool,
    max_keys           int8,
    prefix             text,
    start_after        text,
    region             text
);

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


create function
    aws_request(request s3_list_objects_v2,
                access_key_id text,
                secret_access_key text,
                region text default 'us-east-1',
                endpoint_url text default null)
    returns omni_httpc.http_request
    language plpgsql
    immutable
as
$$
declare
    payload text := '';
    query   text := '';
    params  text[];
    ts8601 timestamp with time zone := now();
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


    if not request.path like '/%' then
        request.path := '/' || request.path;
    end if;

    if endpoint_url is null then
        endpoint_url := omni_aws.s3_endpoint_url(request.bucket, region);
    else
        request.path := '/' || request.bucket || request.path;
    end if;

    if request.region is not null then
        region := request.region;
    end if;

    return omni_httpc.http_request(endpoint_url ||
                                   request.path || (case when length(query) > 0 then '?' || query else '' end),
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
                                                        array ['host:' || substring(endpoint_url from '^[a-zA-Z]+://(.*)'),
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

create type s3_storage_class as enum ('STANDARD', 'REDUCED_REDUNDANCY', 'GLACIER', 'STANDARD_IA', 'ONEZONE_IA' , 'INTELLGIENT_TIERING', 'DEEP_ARCHIVE', 'OUTPOSTS', 'GLACIER_IR', 'SNOW');
create type s3_checksum_algorithm as enum ('CRC32', 'CRC32C', 'SHA1', 'SHA256');
create type s3_owner as
(
    display_name text,
    id           text
);
create type s3_restore_status as
(
    is_restore_in_progress bool,
    restore_expiry_date    timestamp
);

create type s3_list_objects_v2_meta as
(
    bucket_name             text,
    prefix                  text,
    is_truncated            bool,
    continuation_token      text,
    next_continuation_token text,
    delimiter               text,
    common_prefixes         text[],
    start_after             text,
    encoding_type           text,
    request_charged         bool
);

create function aws_execute(
    access_key_id text,
    secret_access_key text,
    region text default 'us-east-1',
    endpoint_url text default null,
    request s3_list_objects_v2 default null)
    returns table
            (
                key                text,
                etag               text,
                last_modified      timestamp,
                size               int8,
                storage_class      s3_storage_class,
                checksum_algorithm s3_checksum_algorithm,
                owner              s3_owner,
                restore_status     s3_restore_status,
                meta s3_list_objects_v2_meta
            )
    language plpgsql
as
$$
declare
    rec record;
begin
    for rec in select *
               from
    omni_aws.aws_execute(access_key_id => access_key_id, secret_access_key := secret_access_key, region := region,
                         endpoint_url := endpoint_url, requests => array [request])
        loop
            if rec.error is not null then
                raise '%', rec.error;
            end if;
            key := rec.key;
            etag := rec.etag;
            last_modified := rec.last_modified;
            size = rec.size;
            storage_class = rec.storage_class;
            checksum_algorithm := rec.checksum_algorithm;
            owner := rec.owner;
            restore_status := rec.restore_status;
            meta := rec.meta;
            return next;
        end loop;
end
$$;

create function aws_execute(
    access_key_id text,
    secret_access_key text,
    region text default 'us-east-1',
    endpoint_url text default null,
    requests s3_list_objects_v2[] default array []::s3_list_objects_v2[])
    returns table
            (
                key                text,
                etag               text,
                last_modified      timestamp,
                size               int8,
                storage_class      s3_storage_class,
                checksum_algorithm s3_checksum_algorithm,
                owner              s3_owner,
                restore_status s3_restore_status,
                meta  s3_list_objects_v2_meta,
                error text
            )
    language plpgsql
as
$$
declare
    _region text := region;
    req     record;
    rec             record;
    xml_node text;
begin
    for req in
select
    convert_from(body, 'utf-8') as resp,
    http_execute.headers,
    http_execute.error
from
    omni_httpc.http_execute(
            variadic (select
                          array_agg(omni_aws.aws_request(request => r.*, access_key_id => access_key_id,
                                                         secret_access_key => secret_access_key,
                                                         region => _region, endpoint_url => endpoint_url))
                      from
                          unnest(requests) r))
        loop
            if req.error is null then
                select value from omni_xml.xpath(req.resp, '/Error/Code/text()') into error;
                if found then
                    key := null;
                    etag := null;
                    last_modified := null;
                    size := null;
                    storage_class := null;
                    checksum_algorithm := null;
                    owner := null;
                    restore_status := null;
                    meta := null;
                    return next;
                    continue;
                end if;
                perform
                from
                    unnest(req.headers) as hdrs
                where
                    hdrs.name = 'x-amz-request-charged';
                meta.request_charged := found;
                select value from omni_xml.xpath(req.resp, '/ListBucketResult/Name/text()') into meta.bucket_name;
                select value from omni_xml.xpath(req.resp, '/ListBucketResult/Prefix/text()') into meta.prefix;
                select
                    array_agg(value)
                from
                    omni_xml.xpath(req.resp, '/ListBucketResult/CommnPrefixes/Prefix/text()')
                into meta.prefix;
                select value from omni_xml.xpath(req.resp, '/ListBucketResult/Delimiter/text()') into meta.delimiter;
                select
                    value
                from
                    omni_xml.xpath(req.resp, '/ListBucketResult/IsTruncated/text()')
                into meta.is_truncated;
                select
                    value
                from
                    omni_xml.xpath(req.resp, '/ListBucketResult/ContinuationToken/text()')
                into meta.continuation_token;
                select
                    value
                from
                    omni_xml.xpath(req.resp, '/ListBucketResult/NextContinuationToken/text()')
                into meta.next_continuation_token;
                select
                    value
                from
                    omni_xml.xpath(req.resp, '/ListBucketResult/EncodingType/text()')
                into meta.encoding_type;
                select value from omni_xml.xpath(req.resp, '/ListBucketResult/StartAfter/text()') into meta.start_after;

                for rec in select value from omni_xml.xpath(req.resp, '/ListBucketResult/Contents')
                    loop
                        select value from omni_xml.xpath(rec.value, '/Contents/Key/text()') into key;
                        select value from omni_xml.xpath(rec.value, '/Contents/ETag/text()') into etag;
                        select value from omni_xml.xpath(rec.value, '/Contents/LastModified/text()') into last_modified;
                        select value from omni_xml.xpath(rec.value, '/Contents/Size/text()') into size;
                        select value from omni_xml.xpath(rec.value, '/Contents/StorageClass/text()') into storage_class;
                        select
                            value
                        from
                            omni_xml.xpath(rec.value, '/Contents/ChecksumAlgorithm/text()')
                        into checksum_algorithm;
                        select value from omni_xml.xpath(rec.value, '/Contents/Owner') into xml_node;
                        if xml_node is not null then
                        select value from omni_xml.xpath(xml_node, '/Owner/DisplayName/text()') into owner.display_name;
                        select value from omni_xml.xpath(xml_node, '/Owner/ID/text()') into owner.id;
                        end if;
                        select value from omni_xml.xpath(rec.value, '/Contents/RestoreStatus') into xml_node;
                        if found then
                            select
                                value
                            from
                                omni_xml.xpath(xml_node, '/RestoreStatus/IsRestoreInProgress/text()')
                            into restore_status.is_restore_in_progress;
                            select
                                value
                            from
                                omni_xml.xpath(xml_node, '/RestoreStatus/RestoreExpiryDate/text()')
                            into restore_status.restore_expiry_date;
                        end if;
                        return next;
                    end loop;
            else
                error := req.error;
                key := null;
                etag := null;
                last_modified := null;
                size := null;
                storage_class := null;
                checksum_algorithm := null;
                owner := null;
                restore_status := null;
                meta := null;
                return next;
            end if;
        end loop;

return;
end
$$;

create type s3_put_object as
(
    bucket       text,
    path         text,
    payload      bytea,
    content_type text,
    region       text
);

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

create function aws_request(request s3_put_object,
                            access_key_id text,
                            secret_access_key text,
                            region text default 'us-east-1',
                            endpoint_url text default null)
    returns omni_httpc.http_request
    language plpgsql
    immutable
as
$$
declare
    ts8601 timestamp with time zone := now();
begin


    if not request.path like '/%' then
        request.path := '/' || request.path;
    end if;

    if endpoint_url is null then
        endpoint_url := omni_aws.s3_endpoint_url(request.bucket, region);
    else
        request.path := '/' || request.bucket || request.path;
    end if;

    if request.region is not null then
        region := request.region;
    end if;

    return omni_httpc.http_request(endpoint_url || request.path,
                                   method => 'PUT',
                                   body => request.payload,
                                   headers => array [
                                       omni_http.http_header('Content-Type', request.content_type),
                                       omni_http.http_header(
                                               'X-Amz-Content-Sha256',
                                               encode(digest(request.payload, 'sha256'), 'hex')),
                                       omni_http.http_header(
                                               'X-Amz-Date',
                                               to_char((ts8601 at time zone 'UTC'),
                                                       'YYYYMMDD"T"HH24MISS"Z"')),
                                       omni_http.http_header(
                                               'Authorization',
                                               'AWS4-HMAC-SHA256 Credential=' || access_key_id || '/'
                                                   || to_char((ts8601 at time zone 'UTC'), 'YYYYMMDD') ||
                                               '/' || region ||
                                               '/s3/aws4_request, SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date;content-type, Signature='
                                                   || omni_aws.hash_string_to_sign(
                                                       'AWS4-HMAC-SHA256',
                                                       ts8601,
                                                       region,
                                                       's3',
                                                       omni_aws.hash_canonical_request(
                                                               'PUT',
                                                               request.path,
                                                               '',
                                                               array [
                                                                   'content-type:' || request.content_type,
                                                                       'host:'
                                                                       ||
                                                                       substring(endpoint_url from '^[a-zA-Z]+://(.*)'),
                                                                       'x-amz-content-sha256:' ||
                                                                       encode(digest(request.payload, 'sha256'), 'hex'),
                                                                       'x-amz-date:' ||
                                                                       to_char((ts8601 at time zone 'UTC'),
                                                                               'YYYYMMDD"T"HH24MISS"Z"')
                                                                   ],
                                                               '{"content-type", "host", "x-amz-content-sha256", "x-amz-date"}',
                                                               encode(digest(request.payload, 'sha256'), 'hex')
                                                           ),
                                                       secret_access_key
                                                   )
                                           ) ]
        );
end;
$$;

create function aws_execute(
    access_key_id text,
    secret_access_key text,
    region text default 'us-east-1',
    endpoint_url text default null,
    request s3_put_object default null)
    returns void
    language plpgsql
as
$$
declare
    rec record;
begin
    for rec in select *
               from
                   omni_aws.aws_execute(access_key_id => access_key_id, secret_access_key := secret_access_key,
                                        region := region,
                                        endpoint_url := endpoint_url, requests => array [request])
        loop
            if rec.error is not null then
                raise '%', rec.error;
            end if;
        end loop;
    return;
end;
$$;

create function aws_execute(
    access_key_id text,
    secret_access_key text,
    region text default 'us-east-1',
    endpoint_url text default null,
    requests s3_put_object[] default array []::s3_put_object[])
    returns table
            (
                error text
            )
    language plpgsql
as
$$
declare
    _region text := region;
    req     record;
begin
    for req in
        select
            http_execute.status,
            http_execute.headers,
            http_execute.body
        from
            omni_httpc.http_execute(
                    variadic (select
                                  array_agg(omni_aws.aws_request(request => r.*, access_key_id => access_key_id,
                                                                 secret_access_key => secret_access_key,
                                                                 region => _region, endpoint_url => endpoint_url))
                              from
                                  unnest(requests) r))
        loop
            if req.status = 200 then
                error := null;
            else
                error := req.status || convert_from(req.body, 'utf-8');
            end if;
            return next;
        end loop;
    return;
end;
$$;

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


create type s3_create_bucket as
(
    bucket text,
    region text
);

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

create function aws_request(request s3_create_bucket,
                            access_key_id text,
                            secret_access_key text,
                            region text default 'us-east-1',
                            endpoint_url text default null)
    returns omni_httpc.http_request
    language plpgsql
    immutable
as
$$
declare
    ts8601  timestamp with time zone := now();
    path    text                     := '/';
    payload bytea;
begin
    if endpoint_url is null then
        endpoint_url := omni_aws.s3_endpoint_url(request.bucket, region);
    else
        path := '/' || request.bucket;
    end if;

    if request.region is not null then
        region := request.region;
    end if;

    payload :=
            convert_to(
                    '<CreateBucketConfiguration xmlns="http://s3.amazonaws.com/doc/2006-03-01/"></CreateBucketConfiguration>',
                    'utf-8');

    return omni_httpc.http_request(endpoint_url || path,
                                   method => 'PUT',
                                   body => payload,
                                   headers => array [
                                       omni_http.http_header(
                                               'Content-Type',
                                               'application/xml'),
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
                                               '/s3/aws4_request, SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date;content-type, Signature='
                                                   || omni_aws.hash_string_to_sign(
                                                       'AWS4-HMAC-SHA256',
                                                       ts8601,
                                                       region,
                                                       's3',
                                                       omni_aws.hash_canonical_request(
                                                               'PUT',
                                                               path,
                                                               '',
                                                               array [
                                                                   'content-type:application/xml',
                                                                       'host:'
                                                                       ||
                                                                       substring(endpoint_url from '^[a-zA-Z]+://(.*)'),
                                                                       'x-amz-content-sha256:' ||
                                                                       encode(digest(payload, 'sha256'), 'hex'),
                                                                       'x-amz-date:' ||
                                                                       to_char((ts8601 at time zone 'UTC'),
                                                                               'YYYYMMDD"T"HH24MISS"Z"')
                                                                   ],
                                                               '{"content-type","host", "x-amz-content-sha256", "x-amz-date"}',
                                                               encode(digest(payload, 'sha256'), 'hex')
                                                           ),
                                                       secret_access_key
                                                   )
                                           ) ]
        );
end;
$$;

create function aws_execute(
    access_key_id text,
    secret_access_key text,
    region text default 'us-east-1',
    endpoint_url text default null,
    request s3_create_bucket default null)
    returns void
    language plpgsql
as
$$
declare
    rec record;
begin
    for rec in select *
               from
                   omni_aws.aws_execute(access_key_id => access_key_id, secret_access_key := secret_access_key,
                                        region := region,
                                        endpoint_url := endpoint_url, requests => array [request])
        loop
            if rec.error is not null then
                raise '%', rec.error;
            end if;
        end loop;
    return;
end;
$$;

create function aws_execute(
    access_key_id text,
    secret_access_key text,
    region text default 'us-east-1',
    endpoint_url text default null,
    requests s3_create_bucket[] default array []::s3_create_bucket[])
    returns table
            (
                error text
            )
    language plpgsql
as
$$
declare
    _region text := region;
    req     record;
begin
    for req in
        select
            http_execute.status,
            http_execute.headers,
            http_execute.body
        from
            omni_httpc.http_execute(
                    variadic (select
                                  array_agg(omni_aws.aws_request(request => r.*, access_key_id => access_key_id,
                                                                 secret_access_key => secret_access_key,
                                                                 region => _region, endpoint_url => endpoint_url))
                              from
                                  unnest(requests) r))
        loop
            if req.status = 200 then
                error := null;
            else
                error := req.status || convert_from(req.body, 'utf-8');
            end if;
            return next;
        end loop;
    return;
end;
$$;