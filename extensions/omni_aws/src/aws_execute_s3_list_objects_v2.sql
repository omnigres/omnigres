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
                meta               s3_list_objects_v2_meta
            )
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
                restore_status     s3_restore_status,
                meta               s3_list_objects_v2_meta,
                error              text
            )
    language plpgsql
as
$$
declare
    _region  text := region;
    req      record;
    rec      record;
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
                            select value
                            from omni_xml.xpath(xml_node, '/Owner/DisplayName/text()')
                            into owner.display_name;
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
