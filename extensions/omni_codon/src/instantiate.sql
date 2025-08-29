create function instantiate(schema regnamespace default 'omni_codon') returns void
    language plpgsql
as
$$
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    create type codon;

    create function codon_call_handler()
        returns language_handler
        language c
    as
    'MODULE_PATHNAME';

    create language plcodon handler codon_call_handler;

end;
$$;
