create function omni_id_uuidv7() returns uuid
    language c as
'MODULE_PATHNAME',
'_uuidv7';

do
$$
    declare
        _p regproc;
    begin
        _p := 'uuidv7()'::regprocedure;
        -- Don't create if it is already defined
        execute $sql$ drop function omni_id_uuidv7 $sql$;
    exception
        when others then
            execute $sql$ alter function omni_id_uuidv7 rename to uuidv7 $sql$;
    end;
$$;
