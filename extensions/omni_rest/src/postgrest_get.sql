create function postgrest_convert_operator (operator_name text)
    returns text immutable
    language sql
    as $$
    select
        case operator_name
        when 'eq' then
            '='
        when 'neq' then
            '<>'
        when 'isdistinct' then
            'is distinct from '
        when 'gt' then
            '>'
        when 'lt' then
            '<'
        when 'gte' then
            '>='
        when 'lte' then
            '<='
        when 'like' then
            'like'
        when 'ilike' then
            'ilike'
        when 'match' then
            '~'
        when 'imatch' then
            '~*'
        when 'not' then
            'not'
        end;
$$;

create or replace function postgrest_parse_logical (input text)
    returns jsonb
    language plpgsql
    as $$
declare
    result jsonb := '[]';
    -- initialize an empty json array
    current text := '';
    current_production text := '';
    depth int := 0;
    token text;
begin
    -- no nesting, just split and return
    if input !~ '[()]' then
        select
            jsonb_agg(jsonb_build_object('operator', omni_rest.postgrest_convert_operator (split_part(term, '.', 2)), 'operands', jsonb_build_array(split_part(term, '.', 1), split_part(term, '.', 3))))
        from
            unnest(string_to_array(input, ',')) term into result;
        return result;
    end if;
    -- iterate through the string character by character
    foreach token in array regexp_split_to_array(input, '')
    loop
        if token = '(' then
            if depth > 0 then
                current := current || token;
            end if;
            depth := depth + 1;
        elsif token = ')' then
            depth := depth - 1;
            if depth > 0 then
                current := current || token;
            end if;
            -- if we finish a nested expression, process it
            if depth = 0 then
                result := result || case lower(current_production)
                when '' then
                    omni_rest.postgrest_parse_logical (current)
                when 'or' then
                    jsonb_build_object('operator', 'or', 'operands', omni_rest.postgrest_parse_logical (current))
                when 'and' then
                    jsonb_build_object('operator', 'and', 'operands', omni_rest.postgrest_parse_logical (current))
                when 'not.or' then
                    jsonb_build_object('operator', 'not', 'operands', jsonb_build_array(jsonb_build_object('operator', 'or', 'operands', omni_rest.postgrest_parse_logical (current))))
                when 'not.and' then
                    jsonb_build_object('operator', 'not', 'operands', jsonb_build_array(jsonb_build_object('operator', 'and', 'operands', omni_rest.postgrest_parse_logical (current))))
                when 'not' then
                    jsonb_build_object('operator', 'not', 'operands', omni_rest.postgrest_parse_logical (current))
                end;
                current := '';
            end if;
        elsif depth > 0 then
            current := current || token;
        end if;
        if depth = 0 then
            if token = ',' then
                if current_production !~ '[()]' then
                    result := result || jsonb_build_array(jsonb_build_object('operator', omni_rest.postgrest_convert_operator (split_part(current_production, '.', 2)), 'operands', jsonb_build_array(split_part(current_production, '.', 1), split_part(current_production, '.', 3))));
                end if;
                current_production := '';
            else
                current_production := current_production || token;
            end if;
        end if;
    end loop;
    if current_production !~ '[()]' then
        result := result || jsonb_build_array(jsonb_build_object('operator', omni_rest.postgrest_convert_operator (split_part(current_production, '.', 2)), 'operands', jsonb_build_array(split_part(current_production, '.', 1), split_part(current_production, '.', 3))));
    end if;
    return result;
end;
$$;

create function postgrest_parse_get_param (params text[])
    returns jsonb immutable
    language sql
    as $$
    with indexed_params as (
        select
            *
        from
            unnest(params)
            with ordinality r (param, index)
),
comparisons as (
select
    lower(columns.param) as key,
    values.param as value,
    omni_rest.postgrest_convert_operator (split_part(values.param, '.', 1)) as first_operator,
    omni_rest.postgrest_convert_operator (split_part(values.param, '.', 2)) as second_operator
from
    indexed_params columns
    join indexed_params
values
    on columns.index % 2 = 1
        and values.index % 2 = 0
        and columns.index = values.index - 1
)
select
    jsonb_build_object('operator', 'and', 'operands', coalesce(jsonb_agg(
                case when key = 'not.and' then
                    jsonb_build_object('operator', 'not', 'operands', jsonb_build_array(jsonb_build_object('operator', 'and', 'operands', omni_rest.postgrest_parse_logical (value))))
                when key = 'and' then
                    jsonb_build_object('operator', 'and', 'operands', omni_rest.postgrest_parse_logical (value))
                when key = 'not.or' then
                    jsonb_build_object('operator', 'not', 'operands', jsonb_build_array(jsonb_build_object('operator', 'or', 'operands', omni_rest.postgrest_parse_logical (value))))
                when key = 'or' then
                    jsonb_build_object('operator', 'or', 'operands', omni_rest.postgrest_parse_logical (value))
                when first_operator = 'not' then
                    jsonb_build_object('operator', 'not', 'operands', jsonb_build_array(jsonb_build_object('operator', second_operator, 'operands', jsonb_build_array(key, split_part(value, '.', 3)))))
                when first_operator is not null then
                    jsonb_build_object('operator', first_operator, 'operands', jsonb_build_array(key, split_part(value, '.', 2)))
                else
                    jsonb_build_object('operator', 'invalid_operator', 'operands', '[]'::jsonb)
                end), '[]'::jsonb))
from
    comparisons
where
    key not in ('select', 'order')
$$;

create function postgrest_format_get_param (where_ast jsonb)
    returns text immutable
    language plpgsql
    as $$
declare
    output text;
begin
    select
        case where_ast ->> 'operator'
        when 'invalid_operator' then
            'invalid_operator invalid_operator'
        when 'not' then
            format('not (%1$s)', omni_rest.postgrest_format_get_param (where_ast -> 'operands' -> 0))
        when 'and' then
        (
            select
                string_agg(omni_rest.postgrest_format_get_param (op), ' and ')
            from
                jsonb_array_elements(where_ast -> 'operands') op)
        when 'or' then
        (
            select
                string_agg(omni_rest.postgrest_format_get_param (op), ' or ')
            from
                jsonb_array_elements(where_ast -> 'operands') op)
    else
        format('%1$I %2$s %3$L', where_ast -> 'operands' ->> 0, where_ast ->> 'operator', where_ast -> 'operands' ->> 1)
        end into output;
    return output;
end;
$$;

create procedure postgrest_get(request omni_httpd.http_request, outcome inout omni_httpd.http_outcome,
                               settings postgrest_settings default postgrest_settings())
    language plpgsql as
$$
declare
    namespace text;
    query_columns   text[];
    query     text;
    result   jsonb;
    params    text[];
    col       text;
    _select   text;
    _where   text;
    _order   text;
    _offset numeric;
    relation regclass;
    request_cache text;
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

    request_cache := 'omni_rest.query_' ||
                     encode(digest(relation || ' ' || coalesce(request.query_string, ''), 'sha256'), 'hex');
    query := omni_var.get_session(request_cache, null::text);
    if query is null then

        params := omni_web.parse_query_string(request.query_string);

        -- Columns (vertical filtering)
        query_columns := array ['*'];

        _select = omni_web.param_get(params, 'select');

        if _select is not null and _select != '' then
            query_columns := array []::text[];

            foreach col in array string_to_array(_select, ',')
                loop
                    declare
                        col_name    text;
                        col_expr    text;
                        col_alias   text;
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
                                    regexp_replace(col_expr, '(->>?)([[:alpha:]_][[:alnum:]_]*)(?=(->|$))', '\1''\2''',
                                                   'g');
                            -- TODO: record access using ->
                        end if;
                        -- TODO: sanitize col_expr
                        query_columns := query_columns || (relation || '.' || col_expr || ' as "' || col_alias || '"');
                    end;
                end loop;
        end if;

        _where := omni_rest.postgrest_format_get_param(omni_rest.postgrest_parse_get_param(params));

        _offset := 0;
        select 'ORDER BY ' || string_agg(format('%1$I %2$s', split_part(o, '.', 1),
                                             -- FIXME: we should error instead of using a default on invalid ordering clauses
                                                case lower(split_part(o, '.', 2))
                                                    when 'desc' then
                                                        'desc'
                                                    else
                                                        'asc'
                                                    end), ',')
        from unnest(regexp_split_to_array(omni_web.param_get(params, 'order'), ',')) o
        into _order;
        -- Finalize the query
        query := format('select %3$s from %1$I.%2$I where %4$s %5$s', namespace, (
            select
                relname
            from pg_class
            where
                oid = relation), concat_ws(', ', variadic query_columns), coalesce(_where, 'true'), _order);
        perform omni_var.set_session(request_cache, query);
    end if;
    -- Run it
    declare
      message text;
      detail text;
      hint text;
    begin
        select
            coalesce(jsonb_agg(stmt_row), '[]'::jsonb) into result
        from
            omni_sql.execute (query);
        outcome := omni_httpd.http_response (result, headers => array[omni_http.http_header ('Content-Range', _offset || '-' || jsonb_array_length(result) || '/*')]);
    exception
        when others then
            get stacked diagnostics message = message_text,
            detail = pg_exception_detail,
            hint = pg_exception_hint;
    outcome := omni_httpd.http_response (jsonb_build_object('message', message, 'detail', detail, 'hint', hint), status => 400);
    end;

end;
$$;
