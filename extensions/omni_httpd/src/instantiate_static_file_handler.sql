create function instantiate_static_file_handler(schema name default '@extschema@',
                                                name name default 'static_file_handler') returns void
    language plpgsql as
$$
declare
    old_search_path text := current_setting('search_path');
begin
    -- check for required extensions
    perform from pg_extension where extname = 'omni_vfs';
    if not found then
        raise exception 'omni_vfs extension is required to be installed';
    end if;

    perform from pg_extension where extname = 'omni_mimetypes';
    if not found then
        raise exception 'omni_mimetypes extension is required to be installed';
    end if;

    perform set_config('search_path', schema::text, true);

    /*{% include "static_file_handler.sql" %}*/

    if name != 'static_file_handler' then
        execute format('alter procedure static_file_handler rename to %I', name);
    end if;

    create table static_file_handler_router (

    );

    perform set_config('search_path', old_search_path, true);
end;
$$;
