-- API: PUBLIC
create function docker_container_inspect(id text)
    returns jsonb
as
$$
declare
response omni_httpc.http_response;
begin
    select * into response from omni_httpc.http_execute(
        omni_httpc.http_request(
            format('http://%s/containers/%s/json', omni_containers.docker_api_base_url(), id)
        )
    );
    case response.status
        when 200 then
            return convert_from(response.body, 'UTF8')::jsonb;
        else
            raise exception 'Can''t inspect the container' using
                detail = format('Error code %s: %s', response.status, convert_from(response.body, 'UTF8')::jsonb);
    end case;
end;
$$ language plpgsql;