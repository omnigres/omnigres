create function pod_credentials()
    returns table
            (
                token  text,
                cacert text
            )
    security definer
    language plpgsql
as
$$
begin
    if pg_stat_file('/var/run/secrets/kubernetes.io/serviceaccount/token', true) is distinct from null then
        token := pg_read_file('/var/run/secrets/kubernetes.io/serviceaccount/token');
    end if;
    if pg_stat_file('/var/run/secrets/kubernetes.io/serviceaccount/ca.crt', true) is distinct from null then
        cacert := pg_read_file('/var/run/secrets/kubernetes.io/serviceaccount/ca.crt');
    end if;
    if not (token is null and cacert is null) then
        return next;
    end if;
    return;
end;
$$;