create function _validate_connstr(connstr text) returns void
    security invoker
    language plpgsql
as
$$
begin
    begin
        perform dblink_connect('dump_' || connstr, connstr);
        perform dblink_disconnect('dump_' || connstr);
    exception
        when others then
            raise exception 'Invalid connection string: %', sqlerrm;
    end;
    return;
end;
$$;
