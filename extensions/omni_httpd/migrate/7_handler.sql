-- Empty implementation
create function handler(int, http_request) returns http_outcome
    language plpgsql as
$$
begin
    return omni_httpd.http_response(status => 404);
end;
$$;

comment on function handler(int, http_request) is $$
This function responds to all HTTP requests. Can't be deleted but can be replaced.
$$;

-- Effectively, soft-deprecate omni_httpd.handlers by wrapping them into the `handler`
/*{% include "../src/regenerate_handler_trigger.sql" %}*/

create trigger deprecated_handlers_to_handler_generator
    after insert or update or delete or truncate
    on handlers
    for each statement
execute function regenerate_handler_trigger();

create trigger deprecated_listeners_handlers_to_handler_generator
    after insert or update or delete or truncate
    on listeners_handlers
    for each statement
execute function regenerate_handler_trigger();

drop trigger handlers_updated on handlers;
drop trigger listeners_handlers_updated on listeners_handlers;
