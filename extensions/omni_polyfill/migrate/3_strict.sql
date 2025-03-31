do
$$
    declare
        _p regproc;
    begin
        begin
            _p := 'omni_polyfill.uuid_extract_version'::regproc;
            alter function omni_polyfill.uuid_extract_version strict;
        exception
            when others then
            null;
        end;
        begin
            _p := 'omni_polyfill.uuid_extract_timestamp'::regproc;
            alter function omni_polyfill.uuid_extract_timestamp strict;
        exception
            when others then
            null;
        end;
    end;
$$;
