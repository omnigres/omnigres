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