-- Basic implementation
create function websocket_handler(int, uuid, http_request) returns bool
    language plpgsql as
$$
    begin
        return false;
    end;
$$;

comment on function websocket_handler(int, uuid, http_request) is $$
This function responds to WebSocket upgrade attempts. If it returns anything but true, the connection is rejected.

Can't be deleted but can be replaced.
$$;


create function websocket_on_open(uuid) returns void
    language plpgsql as
$$
begin
end;
$$;

comment on function websocket_on_open(uuid) is $$
This function is called when WebSocket connection is open.

Can't be deleted but can be replaced.
$$;

comment on function websocket_handler(int, uuid, http_request) is $$
This function responds to WebSocket upgrade attempts. If it returns anything but true, the connection is rejected.

Can't be deleted but can be replaced.
$$;


create function websocket_on_close(uuid) returns void
    language plpgsql as
$$
begin
end;
$$;

comment on function websocket_on_close(uuid) is $$
This function is called when WebSocket connection is close.

Can't be deleted but can be replaced.
$$;

create function websocket_on_message(uuid, bytea) returns void
    language plpgsql as
$$
begin
end;
$$;

create function websocket_on_message(uuid, text) returns void
    language plpgsql as
$$
begin
end;
$$;


comment on function websocket_on_message(uuid, bytea) is $$
This function handles WebSocket binary messages.

Can't be deleted but can be replaced.
$$;

comment on function websocket_on_message(uuid, text) is $$
This function handles WebSocket text messages.

Can't be deleted but can be replaced.
$$;

create function websocket_send(uuid, text) returns void
    language c as
'MODULE_PATHNAME',
'websocket_send_text';

create function websocket_send(uuid, bytea) returns void
    language c as
'MODULE_PATHNAME',
'websocket_send_bytea';