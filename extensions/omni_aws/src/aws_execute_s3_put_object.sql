create function aws_execute(
    access_key_id text,
    secret_access_key text,
    region text default 'us-east-1',
    endpoint s3_endpoint default omni_aws.aws_s3_endpoint(),
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
