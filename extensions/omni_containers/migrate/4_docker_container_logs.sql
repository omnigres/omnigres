-- API: PUBLIC
create function docker_stream_to_text(
    stream bytea
)
    returns text
as
'MODULE_PATHNAME',
'docker_stream_to_text'
    language c;

create function docker_container_logs(
    id text,
    stdout bool default true,
    stderr bool default true,
    since timestamp default null,
    until timestamp default null,
    timestamps bool default false,
    tail int default null
)
    returns text
as
$$
declare
query_params text = '';
response omni_httpc.http_response;
begin
    if stdout = true then
        query_params = query_params || '&stdout=true';
    else
        query_params = query_params || '&stdout=false';
    end if;

    if stderr = true then
        query_params = query_params || '&stderr=true';
    else
        query_params = query_params || '&stderr=false';
    end if;

    if since is not null then
        query_params = query_params || format('&since=%s', extract(epoch from since)::bigint);
    end if;
    
    if until is not null then
        query_params = query_params || format('&until=%s', extract(epoch from until)::bigint);
    end if;

    if timestamps = true then
        query_params = query_params || '&timestamps=true';
    else
        query_params = query_params || '&timestamps=false';
    end if;

    if tail is null or tail = -1 then
        query_params = query_params || '&tail=all';
    else
        query_params = query_params || format('&tail=%s', tail);
    end if;

    select * into response from omni_httpc.http_execute(
        omni_httpc.http_request(
            format('http://%s/containers/%s/logs?', omni_containers.docker_api_base_url(), id) || query_params
        )
    );

    if response.status = 200 then
        return omni_containers.docker_stream_to_text(response.body);
    else
        raise exception 'Can''t get logs from the container' using
        detail = format('Error code %s: %s', response.status, convert_from(response.body, 'UTF8'));
    end if;
end;
$$ language plpgsql;