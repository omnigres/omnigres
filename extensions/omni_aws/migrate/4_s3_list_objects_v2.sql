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
                endpoint s3_endpoint default omni_aws.aws_s3_endpoint())
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
    endpoint s3_endpoint default null,
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
                                        endpoint => endpoint, requests => array [request])
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
    endpoint s3_endpoint default null,
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
                                                                 region => _region, endpoint => endpoint))
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
