create procedure postgrest_get(request omni_httpd.http_request, outcome inout omni_httpd.http_outcome,
                               settings postgrest_settings default postgrest_settings())
    language plpgsql as
$$
declare
    namespace text;
    columns   text[];
    query     text;
    result   jsonb;
    params    text[];
    col       text;
    _select   text;
    _offset numeric;
    relation regclass;
begin
    if outcome is distinct from null then
        return;
    end if;
    if request.method = 'GET' then
        call omni_rest._postgrest_relation(request, relation, namespace, settings);
        if relation is null then
            return; -- terminate
        end if;
    else
        return; -- terminate;
    end if;

    params := omni_web.parse_query_string(request.query_string);

    -- Columns (vertical filtering)
    columns := array ['*'];

    _select = omni_web.param_get(params, 'select');

    if _select is not null and _select != '' then
        columns := array []::text[];

    foreach col in array string_to_array(_select, ',')
        loop
            declare
                col_name    text;
                col_expr text;
                col_alias text;
                _match      text[];
                is_col_name bool;
            begin
                col_expr := split_part(col, ':', 1);
                col_alias := split_part(col, ':', 2);

                _match := regexp_match(col_expr, '^([[:alpha:] _][[:alnum:] _]*)(.*)');
                col_name := _match[1];
                if col_name is null then
                    raise exception 'no column specified in %', col_expr;
                end if;

                -- Check if there is such an attribute
                perform from pg_attribute where attname = col_name and attrelid = relation and attnum > 0;
                is_col_name := found;
                -- If not...
                if not is_col_name then
                    -- Is it a function?
                    perform
                    from pg_proc
                    where proargtypes[0] = relation
                      and proname = col_name
                      and pronamespace::text = namespace;
                    if found then
                        -- It is a function
                        col_name := col_name || '((' || relation || '))';
                    end if;
                end if;
                col_expr := col_name || _match[2];

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
                columns := columns || (relation || '.' || col_expr || ' as "' || col_alias || '"');
            end;
        end loop;
    end if;

    _offset := 0;

    -- Finalize the query
    query := format('select %3$s from %1$I.%2$I', namespace, (select relname from pg_class where oid = relation),
                    concat_ws(', ', variadic columns));

    -- Run it
    select jsonb_agg(stmt_row)
    into result
    from omni_sql.execute(query);

    outcome := omni_httpd.http_response(result,
                                        headers => array [omni_http.http_header('Content-Range',
                                                                            _offset || '-' ||
                                                                            jsonb_array_length(result) ||
                                                                            '/*')]);
end;
$$;