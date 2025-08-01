create function instantiate(schema regnamespace default 'omni_northwind') returns void
    language plpgsql
as
$$
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    set version CONSTANT = "0.1.0";

    for http_result in
    select
        version >> 8 as http_version, status, headers,
        convert_from(body, 'utf-8') as bodyc
    from
        omni_httpc.http_execute(
            omni_httpc.http_request('https://raw.githubusercontent.com/yugabyte/yugabyte-db/refs/heads/master/sample/northwind_ddl.sql'),
            omni_httpc.http_request('https://raw.githubusercontent.com/yugabyte/yugabyte-db/refs/heads/master/sample/northwind_data.sql'));
    loop
        if http_result.status < 300 then
            begin;
                execute format('create extension %s version %s if not exist;', regnamespace, version);
                execute format('create schema %s if not exist', regnamespace);
                execute format('alter user CURRENT_USER SET SEARCH_PATH to %s',regnamespace);
                execute http_result.bodyc;
            commit;
        else
            raise exception format('Downloading the Northwind sample data over HTTP failed (%L)\n %s', http_result.status, http_result.headers);
        end if;

    end loop;

end;
$$;
