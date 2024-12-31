create procedure postgrest_insert(request omni_httpd.http_request, outcome inout omni_httpd.http_outcome,
                                  settings postgrest_settings default postgrest_settings())
    language plpgsql as
$$
declare
    namespace text;
    query     text;
    result    jsonb;
    relation  regclass;
    preference text;
    _missing   text;
    _return    text := 'minimal';
    _tx        text;
begin
    if outcome is distinct from null then
        return;
    end if;

    if request.method = 'POST' then
        call omni_rest._postgrest_relation(request, relation, namespace, settings);
        if relation is null then
            return; -- terminate
        end if;
    else
        return; -- terminate;
    end if;

    for preference in select regexp_split_to_table(omni_http.http_header_get(request.headers, 'prefer'), ',\s+')
        loop
            declare
                preference_name  text := split_part(preference, '=', 1);
                preference_value text := split_part(preference, '=', 2);
            begin
                case
                    when preference_name = 'missing'
                        then _missing := preference_value;
                    when preference_name = 'return'
                        then _return := preference_value;
                    when preference_name = 'tx'
                        then _tx := preference_value;
                    end case;
            end;
        end loop;

    if omni_http.http_header_get(request.headers, 'content-type') = 'application/json' then
        declare
            payload  jsonb := convert_from(request.body, 'utf8')::jsonb;
            defaults jsonb;
        begin
            if jsonb_typeof(payload) = 'object' then
                payload := jsonb_build_array(payload);
            end if;
            if jsonb_typeof(payload) != 'array' then
                outcome := omni_httpd.http_response(status => 422, body => 'JSON must be object or array');
                return;
            end if;

            -- Handle defaults

            if _missing = 'default' then
                -- Prepare default expressions
                select jsonb_object_agg(attname,
                                        coalesce(pg_get_expr(adbin, adrelid), 'null'))
                into defaults
                from pg_attribute a
                         left join pg_attrdef d
                                   on a.
                                          attrelid = d.adrelid and a.attnum = d.adnum
                where a.attrelid = relation
                  and a.attnum > 0
                  and not a.attisdropped;

                -- Apply them to payload rows
                payload := jsonb_agg(
                                   p || (select jsonb_object_agg(
                                                        _defaults.key,
                                                        omni_rest.__eval(_defaults.value #>> '{}')::jsonb
                                                ) - array(
                                                        select jsonb_object_keys(p)
                                                    )
                                         from jsonb_each(defaults) _defaults)
                           )
                           from jsonb_array_elements(payload) p;
            end if;
            -- Query
            query :=
                    format(
                            'insert into %1$I.%2$I select * from jsonb_populate_recordset(null::%1$I.%2$I, %3$L::jsonb) %4$s',
                            namespace,
                            (select relname from pg_class where oid = relation),
                            payload::json,
                            case when _return = 'representation' then 'returning *' else '' end);
        end;
    end if;

    select jsonb_agg(stmt_row)
    into result
    from omni_sql.execute(query);

    if lower(_tx) = 'rollback' then
        rollback and chain;
    end if;

    outcome := omni_httpd.http_response(status => 201,
                                        body => case when _return = 'representation' then result end);
end;
$$;