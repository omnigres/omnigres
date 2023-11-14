-- API: PUBLIC
create function docker_container_stop(id text)
    returns void
as
$$
declare
    response omni_httpc.http_response;
begin
    select *
    into response
    from
        omni_httpc.http_execute(
                omni_httpc.http_request(
                        format('http://%s/containers/%s/stop', omni_containers.docker_api_base_url(), id),
                        'POST'
                    )
            );
    case response.status
        when 204 then -- We're done
        return;
        when 304 then raise exception 'Can''t stop the container' using
            detail = 'Container already stopped';
        when 404 then raise exception 'Can''t stop the container' using
            detail = 'No such container';
        else raise exception 'Can''t stop the container: % (status % error: %)', convert_from(response.body, 'utf8'), response.status, response.error;
        end case;
end;
$$ language plpgsql;