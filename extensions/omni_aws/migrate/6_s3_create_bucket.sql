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
                            endpoint s3_endpoint default omni_aws.aws_s3_endpoint())
    returns omni_httpc.http_request
    language plpgsql
    immutable
as
$$
declare
    ts8601  timestamp with time zone := now();
    path    text                     := '/';
    payload bytea                    := convert_to('', 'utf-8');
    endpoint_url text;
    endpoint_uri omni_web.uri;
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

    return omni_httpc.http_request(endpoint_url,
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
                                               '/s3/aws4_request, SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date, Signature='
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
                                                                       endpoint_uri.host ||
                                                                       coalesce(':' || endpoint_uri.port, ''),
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
    endpoint s3_endpoint default omni_aws.aws_s3_endpoint(),
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
                   omni_aws.aws_execute(access_key_id => access_key_id, secret_access_key => secret_access_key,
                                        region => region,
                                        endpoint => endpoint, requests => array [request])
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
    endpoint s3_endpoint default omni_aws.aws_s3_endpoint(),
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
                                                                 region => _region, endpoint => endpoint))
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