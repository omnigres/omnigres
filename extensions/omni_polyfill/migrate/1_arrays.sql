do
$$
    declare
        major_version int;
    begin
        major_version := current_setting('server_version_num')::int / 10000;
        if major_version < 14 then
            create function trim_array(anyarray, integer) returns anyarray
                immutable parallel safe
                language c as
            'MODULE_PATHNAME';
            comment on function trim_array(anyarray, integer) is 'remove last N elements of array';
        end if;
    end;
$$;