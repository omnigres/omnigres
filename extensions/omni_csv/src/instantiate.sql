create function instantiate(schema regnamespace default 'omni_csv') returns void
    language plpgsql
as
$instantiate$
declare
    old_search_path text := current_setting('search_path');
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    create function csv_info(csv text)
        returns table
                (
                    column_name text
                )
        language c
    as
    'MODULE_PATHNAME';

    create function parse(csv text)
        returns setof record
        language c as
    'MODULE_PATHNAME';

    create function csv_sfunc(internal, record) returns internal
        language c as
    'MODULE_PATHNAME';

    create function csv_ffunc(internal) returns text
        language c as
    'MODULE_PATHNAME';

    create aggregate csv_agg(record) (sfunc = csv_sfunc, finalfunc = csv_ffunc, stype = internal);

    -- Restore the path
    perform set_config('search_path', old_search_path, true);
end
$instantiate$;
