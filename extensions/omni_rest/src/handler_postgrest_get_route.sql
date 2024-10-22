create function handler(postgrest_get_route) returns omni_httpd.http_outcome
    strict
    language plpgsql as
$$
declare
    namespace text;
    columns   text[];
    query     text;
    response  jsonb;
    params    text[];
    col       text;
    _select   text;
    _offset numeric;
begin
    -- Prepare the naming
    select n.nspname as schema_name
    from pg_class c
             join pg_namespace n on c.relnamespace = n.oid
    where c.oid = $1.relation
    into namespace;

    params := omni_web.parse_query_string($1.request.query_string);

    -- Columns (vertical filtering)
    columns := array ['*'];

    _select = omni_web.param_get(params, 'select');

    if _select is not null and _select != '' then
        columns := array []::text[];

    foreach col in array string_to_array(_select, ',')
        loop
            declare
                col_expr text;
                col_alias text;
            begin
                col_expr := split_part(col, ':', 1);
                col_alias := split_part(col, ':', 2);
                if col_alias = '' then
                    col_alias := col_expr;
                end if;
                if col_expr ~ '->' then
                    -- Handle JSON expression
                    if col_alias = col_expr then
                        -- Update the alias if it was not set
                        with arr as (select regexp_split_to_array(col_expr, '->>?') as data)
                        select data[cardinality(data)]
                        from arr
                        into col_alias;
                    end if;
                    -- Rewrite the expression to match its actual syntax
                    col_expr :=
                            regexp_replace(col_expr, '(->>?)([[:alpha:]_][[:alnum:]_]*)(?=(->|$))', '\1''\2''', 'g');
                    -- TODO: record access using ->
                end if;
                -- TODO: sanitize col_expr
                columns := columns || ($1.relation || '.' || col_expr || ' as "' || col_alias || '"');
            end;
        end loop;
    end if;

    _offset := 0;

    -- Finalize the query
    query := format('select %3$s from %1$I.%2$I', namespace, $1.relation, concat_ws(', ', variadic columns));

    -- Run it
    select jsonb_agg(stmt_row)
    into response
    from omni_sql.execute(query);

    return omni_httpd.http_response(response,
                                    headers => array [omni_http.http_header('Content-Range',
                                                                            _offset || '-' ||
                                                                            jsonb_array_length(response) ||
                                                                            '/*')]);
end;
$$;