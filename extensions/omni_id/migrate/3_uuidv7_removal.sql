do
$$
    begin
        perform
        from pg_proc
        where proname = 'uuidv7'
          and probin = 'MODULE_PATHNAME';
        if found then
            drop function uuidv7;
            begin
                create function uuidv7() returns uuid
                    language 'internal' as
                'uuidv7';
            exception
                when others then
                    null;
            end;
        end if;
    end;
$$;