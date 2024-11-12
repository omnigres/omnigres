-- API: PUBLIC
create function docker_container_exec(id text, cmd text,
                                      attach_stdout boolean default false,
                                      attach_stderr boolean default false,
                                      options jsonb default '{}')
    returns text
as
$$
declare
    response  omni_httpc.http_response;
    post_body jsonb = options;
begin
    if cmd is not null then
        post_body = post_body || jsonb_build_object('Cmd', jsonb_build_array('sh', '-c', cmd));
    end if;
    if attach_stdout then
        post_body = post_body || jsonb_build_object('AttachStdout', true);
    end if;
    if attach_stderr then
        post_body = post_body || jsonb_build_object('AttachStderr', true);
    end if;

    select *
    into response
    from
        omni_httpc.http_execute_with_options(
                omni_httpc.http_execute_options(),
                omni_httpc.http_request(
                        format('http://%s/containers/%s/exec', omni_containers.docker_api_base_url(), id),
                        'POST',
                        array [
                            omni_http.http_header('Content-Type', 'application/json')
                            ],
                        convert_to(post_body::text, 'UTF8')
                )
        );
    case response.status
        when 201 then -- it's prepared
        null;
        when 404 then raise exception 'Can''t exec in the container' using
            detail = 'No such container';
        else raise exception 'Can''t exec in the container (HTTP %)', response.status using detail = (convert_from(response.body, 'utf8')::json) ->> 'message';
        end case;
    id := (convert_from(response.body, 'utf8')::jsonb ->> 'Id');

    post_body := '{}';
    if attach_stdout or attach_stderr then
        post_body = post_body || jsonb_build_object('Detach', false);
    end if;
    select *
    into response
    from
        omni_httpc.http_execute_with_options(
                omni_httpc.http_execute_options(),
                omni_httpc.http_request(
                        format('http://%s/exec/%s/start', omni_containers.docker_api_base_url(), id),
                        'POST',
                        array [
                            omni_http.http_header('Content-Type', 'application/json')
                            ],
                        convert_to(post_body::text, 'UTF8')
                )
        );
    case response.status
        when 200 then -- it's prepared
        if attach_stdout or attach_stderr then
            return omni_containers.docker_stream_to_text(response.body);
        else
            return null;
        end if;
        when 404 then raise exception 'Can''t exec in the container' using
            detail = 'No such container';
        else raise exception 'Can''t exec in the container (HTTP %)', response.status using detail = (convert_from(response.body, 'utf8')::json) ->> 'message';
        end case;
end;
$$ language plpgsql;