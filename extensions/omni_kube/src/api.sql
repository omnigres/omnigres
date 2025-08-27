create function api(paths text[],
                    server text default coalesce(current_setting('omni_kube.server', true),
                                                 'https://kubernetes.default.svc'),
                    cacert text default current_setting('omni_kube.cacert', true),
                    clientcert omni_httpc.client_certificate default row (current_setting('omni_kube.clientcert', true), current_setting('omni_kube.client_private_key', true))::omni_httpc.client_certificate,
                    token text default current_setting('omni_kube.token', true),
                    methods omni_http.http_method[] default null,
                    bodies jsonb[] default null, stream boolean default false
)
    returns table
            (
                response jsonb,
                status   int2
            )
    language plpgsql
as
$$
declare
begin
    if cacert is null then
        select p.cacert into cacert from omni_kube.pod_credentials() p;
    end if;
    if token is null then
        select p.token into token from omni_kube.pod_credentials() p;
    end if;

    return query
        with request as (select (omni_httpc.http_request(
                server || path,
                coalesce(method, 'GET'),
                                        array [
                                            omni_http.http_header('Content-Type', 'application/json'),
                                            omni_http.http_header('Accept', 'application/json'),
                                            omni_http.http_header('Authorization', 'Bearer ' || token)
                                            ],
                convert_to(body::text, 'UTF8')
                                 )) as req
                         from lateral (select unnest(paths)           as path,
                                              unnest(methods)         as method,
                                              unnest(bodies)          as body) d),
             agg_request as (select array_agg(req) agg_req from request)
        select case
                   when response.body = '' then null
                   when stream then
                       to_jsonb(string_to_array(rtrim(convert_from(response.body, 'utf-8'), E'\n'), E'\n')::jsonb[])
                   else convert_from(response.body, 'utf-8')::jsonb end,
               response.status
        from agg_request,
        omni_httpc.http_execute_with_options(
                omni_httpc.http_execute_options(allow_self_signed_cert => cacert is null,
                                                cacerts => case when cacert is null then null else array [cacert] end,
                                                clientcert => case when clientcert is null then null else clientcert end),
                variadic agg_req) response;
end;
$$;

create function api(path text,
                    server text default coalesce(current_setting('omni_kube.server', true),
                                                 'https://kubernetes.default.svc'),
                    cacert text default current_setting('omni_kube.cacert', true),
                    clientcert omni_httpc.client_certificate default row (current_setting('omni_kube.clientcert', true), current_setting('omni_kube.client_private_key', true))::omni_httpc.client_certificate,
                    token text default current_setting('omni_kube.token', true),
                    method omni_http.http_method default 'GET',
                    body jsonb default null, stream boolean default false) returns jsonb
    language plpgsql as
$$
declare
    result          jsonb;
    response_status int2;
    request_digest  text;
    cached_response jsonb;
begin
    if substring(path, 1, 1) != '/' then
        raise exception 'path must start with a leading slash';
    end if;

    request_digest := encode(digest(method || ' ' || server || path || coalesce(cacert, 'NULL_CACERT') ||
                                    coalesce(token, 'NULL_TOKEN') || coalesce(body, 'null'), 'sha256'), 'hex');
    cached_response := omni_var.get_statement('omni_kube.request_' || request_digest, null::jsonb);
    if cached_response is not null then
        return cached_response;
    end if;
    select response, status
    into result, response_status
    from api(array [path], server, cacert, clientcert, token, array [method],
             array [body], stream);
    if response_status >= 400 then
        raise exception '%', result ->> 'reason' using detail = result ->> 'message';
    end if;
    return omni_var.set_statement('omni_kube.request_' || request_digest, result);
end;
$$;
