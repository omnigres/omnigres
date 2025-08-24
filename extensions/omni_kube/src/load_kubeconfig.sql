create function load_kubeconfig(config jsonb, context text default null, local boolean default false)
    returns void as
$$
declare
    context_config jsonb;
    cluster_name   text;
    user_name      text;
    cluster_config jsonb;
    user_config    jsonb;
begin
    -- Get current context unless passed
    context := coalesce(context, config ->> 'current-context');

    -- Find context configuration
    select ctx
    into context_config
    from jsonb_array_elements(config -> 'contexts') as ctx
    where ctx ->> 'name' = context;

    if context_config is null then
        raise exception 'context ''%'' not found', context;
    end if;

    cluster_name := context_config -> 'context' ->> 'cluster';
    user_name := context_config -> 'context' ->> 'user';

    -- Find cluster configuration
    select cluster
    into cluster_config
    from jsonb_array_elements(config -> 'clusters') as cluster
    where cluster ->> 'name' = cluster_name;

    -- Find user configuration
    select "user"
    into user_config
    from jsonb_array_elements(config -> 'users') as "user"
    where "user" ->> 'name' = user_name;

    -- Set server and CA cert
    perform set_config('omni_kube.server',
                       cluster_config -> 'cluster' ->> 'server', local);

    if cluster_config -> 'cluster' ->> 'certificate-authority-data' is not null then
        perform set_config('omni_kube.cacert',
                           convert_from(decode(cluster_config -> 'cluster' ->> 'certificate-authority-data', 'base64'),
                                        'utf8'),
                           local);
    elsif cluster_config -> 'cluster' ->> 'certificate-authority' is not null then
        perform set_config('omni_kube.cacert',
                           pg_read_file(cluster_config -> 'cluster' ->> 'certificate-authority'),
                           local);
    end if;

    -- Handle different authentication methods
    if user_config -> 'user' ->> 'client-certificate-data' is not null then
        -- Embedded certificates
        perform set_config('omni_kube.clientcert',
                           convert_from(decode(user_config -> 'user' ->> 'client-certificate-data', 'base64'), 'utf8'),
                           local);
        perform set_config('omni_kube.client_private_key',
                           convert_from(decode(user_config -> 'user' ->> 'client-key-data', 'base64'), 'utf8'),
                           local);

    elsif user_config -> 'user' ->> 'client-certificate' is not null then
        -- Certificate files
        perform set_config('omni_kube.clientcert',
                           pg_read_file(user_config -> 'user' ->> 'client-certificate'),
                           local);
        perform set_config('omni_kube.client_private_key',
                           pg_read_file(user_config -> 'user' ->> 'client-key'),
                           local);

    elsif user_config -> 'user' ->> 'token' is not null then
        -- Token authentication
        perform set_config('omni_kube.token',
                           user_config -> 'user' ->> 'token',
                           local);

    elsif user_config -> 'user' ->> 'tokenFile' is not null then
        -- Token from file
        perform set_config('omni_kube.token',
                           trim(pg_read_file(user_config -> 'user' ->> 'tokenFile')),
                           local);

    elsif user_config -> 'user' -> 'exec' is not null then
        -- Exec plugin - this would require executing external commands
        raise notice 'Exec plugin authentication detected but not yet supported';
        raise exception 'Exec plugin authentication requires external command execution';

    elsif user_config -> 'user' -> 'auth-provider' is not null then
        -- Auth provider - complex, provider-specific logic needed
        raise notice 'Auth provider detected: %', user_config -> 'user' -> 'auth-provider' ->> 'name';
        raise exception 'Auth provider authentication not yet supported';

    else
        raise exception 'No supported authentication method found in kubeconfig';
    end if;

end;
$$ language plpgsql;

create function load_kubeconfig(config json, context text default null, local boolean default false)
    returns void
    language sql
return load_kubeconfig(config::jsonb, context, local);

create function load_kubeconfig(filename text, context text default null, local boolean default false)
    returns void
    language sql
return load_kubeconfig(omni_yaml.to_json(pg_read_file(filename)), context, local);
