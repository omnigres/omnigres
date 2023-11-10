-- API: PUBLIC
create function docker_container_create(
    image text,
    cmd text default null,
    attach text default 'db.omni',
    start bool default true,
    wait bool default false,
    pull bool default false,
    options jsonb default '{}')
    returns text
as
$$
declare
    normalized_image text;
    post_body jsonb = options;
    conn_info jsonb =  '[]';
    port text = '5432';
    retry_creating bool = false;
    attempted_to_pull bool = false;
    response omni_httpc.http_response;
    container_id text;
begin
    case (length(image) - length(replace(image, '/', '')))
        when 0 then -- No slashes, it's a library
            normalized_image = 'docker.io/library/' || image ;
        when 1 then -- One slash, it's on docker.io
            normalized_image = 'docker.io/' || image;
        else -- More than one slash, it's a normalized URl
            normalized_image = image;
    end case;

    post_body = post_body || jsonb_build_object('Image', normalized_image);

    if cmd is not null then
        post_body = post_body || jsonb_build_object('Cmd', jsonb_build_array('sh', '-c', cmd));
    end if;

    if attach is not null then
        conn_info = conn_info::jsonb || to_jsonb(format('PGUSER=%s', current_user));
        conn_info = conn_info::jsonb || to_jsonb(format('PGHOST=%s', attach));
        conn_info = conn_info::jsonb || to_jsonb(format('PGDATABASE=%s', current_database()));
        select setting into port from pg_settings where name = 'port';
        conn_info = conn_info::jsonb || to_jsonb(format('PGPORT=%s', port));

        post_body = jsonb_set(post_body, '{Env}',
        coalesce(post_body->'Env', '[]')::jsonb || conn_info);

        if post_body->'HostConfig' is null then
            post_body = jsonb_set(post_body, '{HostConfig}', '{"ExtraHosts": []}');
        end if;

        if length(omni_containers.docker_host_ip()) = 0 then
            post_body = jsonb_set(post_body, '{HostConfig,ExtraHosts}', 
                coalesce(post_body->'HostConfig'->'ExtraHosts', '[]')::jsonb || to_jsonb(format('%s:host-gateway',
                attach))
            );
        else
            post_body = jsonb_set(post_body, '{HostConfig,ExtraHosts}',
                coalesce(post_body->'HostConfig'->'ExtraHosts', '[]')::jsonb || to_jsonb(format('%s:%s',
                attach, omni_containers.docker_host_ip()))
            );
        end if;
    end if;

    loop
        retry_creating = false;
        -- Attempt to create the container
        select * into response from omni_httpc.http_execute(omni_httpc.http_request(
            format('http://%s/containers/create', omni_containers.docker_api_base_url()),
            'POST',
            array[
                omni_http.http_header('Content-Type', 'application/json')
            ],
            convert_to(post_body::text, 'UTF8')
        ));

        case
            when response.status = 201 then -- We're done
                container_id = convert_from(response.body, 'UTF8')::jsonb->>'Id';
            when response.status = 404 then -- Not found
                if pull and not attempted_to_pull then -- Try to pull the image if allowed to
                    raise notice 'Pulling image %', image;
                    select * into response from omni_httpc.http_execute(omni_httpc.http_request(
                        format(
                            'http://%s/images/create?fromImage=%s&tag=latest',
                            omni_containers.docker_api_base_url(),
                            omni_web.url_encode(normalized_image)
                        ),
                        'POST')
                    );
                    attempted_to_pull = true;

                    if response.status = 200 then
                        retry_creating = true; -- Try to create the container again
                        continue;
                    elsif response.status != 200 then
                        raise exception 'Failed to pull image %', image using
                        detail = format('Error code %s: %s', response.status, convert_from(response.body, 'UTF8'));
                    end if;
                else
                    raise exception 'Docker image not found' using detail = format('%s', image); -- Attempted, wasn't found
                end if;
            else -- Other error
                raise exception 'Can''t create the container' using
                detail = format('Error code %s: %s', response.status, convert_from(response.body, 'UTF8'));
        end case;
        exit when not retry_creating;
    end loop;

    if start = true then
        select * into response from omni_httpc.http_execute(omni_httpc.http_request(
            format('http://%s/containers/%s/start', omni_containers.docker_api_base_url(), container_id),
            'POST'
        ));
        if response.status != 204 then
            raise exception 'Can''t start the container' using
            detail = format('Error code %s: %s', response.status, response.headers);
        end if;
    end if;

    if wait = true then
        select * into response from omni_httpc.http_execute(omni_httpc.http_request(
            format('http://%s/containers/%s/wait', omni_containers.docker_api_base_url(), container_id),
            'POST'
        ));
        if response.status != 200 then
            raise exception 'Can''t wait for the container' using
            detail = format('Error code %s: %s', response.status, convert_from(response.body, 'UTF8'));
        end if;
    end if;
            
    return container_id;
end;
$$ language plpgsql;