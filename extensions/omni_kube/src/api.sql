create function api(path text,
                    server text default coalesce(current_setting('omni_kube.server', true),
                                                 'https://kubernetes.default.svc'),
                    cacert text default current_setting('omni_kube.cacert', true),
                    token text default current_setting('omni_kube.token', true),
                    method omni_http.http_method default 'GET',
                    body jsonb default null) returns jsonb
    language plpgsql
    stable
as
$$
declare
    response omni_httpc.http_response;
begin
    if substring(path, 1, 1) != '/' then
        raise exception 'path must start with a leading slash';
    end if;
    select *
    into response
    from
        omni_httpc.http_execute_with_options(
                omni_httpc.http_execute_options(allow_self_signed_cert => cacert is null,
                                                cacerts => case when cacert is null then null else array [cacert] end),
                omni_httpc.http_request(
                        format('%1$s%2$s', server, path),
                        method,
                        array [
                            omni_http.http_header('Content-Type', 'application/json'),
                            omni_http.http_header('Accept', 'application/json'),
                            omni_http.http_header('Authorization', 'Bearer ' || token)
                            ],
                        convert_to(body::text, 'UTF8')
                ));
    if response.error is not null then
        raise exception 'error: %', response.error;
    end if;
    return convert_from(response.body, 'utf-8')::jsonb;
end;
$$;
