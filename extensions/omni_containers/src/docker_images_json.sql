-- API: PRIVATE
create function docker_images_json()
    returns jsonb
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
                        format('http://%s/images/json', omni_containers.docker_api_base_url())
                    )
            );
    if response.status = 200 then
        return convert_from(response.body, 'UTF8');
    else
        raise exception 'Can''t list docker images' using
            detail = format('Error code %s: %s', response.status, convert_from(response.body, 'UTF8')::jsonb);
    end if;
end;
$$ language plpgsql;

comment
    on function docker_images_json() is 'Private API';