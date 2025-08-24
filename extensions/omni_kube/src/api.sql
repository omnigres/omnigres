create function api(path text,
                    server text default coalesce(current_setting('omni_kube.server', true),
                                                 'https://kubernetes.default.svc'),
                    cacert text default current_setting('omni_kube.cacert', true),
                    clientcert omni_httpc.client_certificate default row (current_setting('omni_kube.clientcert', true), current_setting('omni_kube.client_private_key', true))::omni_httpc.client_certificate,
                    token text default current_setting('omni_kube.token', true),
                    method omni_http.http_method default 'GET',
                    body jsonb default null) returns jsonb
    language plpgsql
as
$$
declare
    response omni_httpc.http_response;
    request_digest  text;
    cached_response jsonb;
begin
    if cacert is null then
        select p.cacert into cacert from omni_kube.pod_credentials() p;
    end if;
    if token is null then
        select p.token into token from omni_kube.pod_credentials() p;
    end if;
    if substring(path, 1, 1) != '/' then
        raise exception 'path must start with a leading slash';
    end if;


    request_digest := encode(digest(method || ' ' || server || path || coalesce(cacert, 'NULL_CACERT') ||
                                    coalesce(token, 'NULL_TOKEN') || coalesce(body, 'null'), 'sha256'), 'hex');
    cached_response := omni_var.get_statement('omni_kube.request_' || request_digest, null::jsonb);
    if cached_response is not null then
        return cached_response;
    end if;
    select *
    into response
    from
        omni_httpc.http_execute_with_options(
                omni_httpc.http_execute_options(allow_self_signed_cert => cacert is null,
                                                cacerts => case when cacert is null then null else array [cacert] end,
                                                clientcert => case when clientcert is null then null else clientcert end),
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
    body := convert_from(response.body, 'utf-8')::jsonb;
    if response.status >= 400 then
        raise exception '%', body ->> 'reason' using detail = body ->> 'message';
    end if;
    return omni_var.set_statement('omni_kube.request_' || request_digest, body);
end;
$$;
