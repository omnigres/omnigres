do
$$
    declare
        major_version int;
    begin
        major_version := split_part(current_setting('server_version'), '.', 1)::int;
        if major_version < 14 then
            create function trim_array(anyarray, integer) returns anyarray
                immutable parallel safe
                language c as
            'MODULE_PATHNAME';
            comment on function trim_array(anyarray, integer) is 'remove last N elements of array';
        end if;
    end;
$$;