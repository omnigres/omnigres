/*
 * Given a relation reference and an array of argument names,
 * it builds a string with placeholders for each argument cast to its type.
 * The placeholders are in the order of the relation definition.
 * This can be used with _postgrest_update_ordered_field_values to build a SQL command that will update the referenced relation.
 */ 
create or replace function _postgrest_update_fields(relation regclass, field_names text[])
    returns text
    immutable
    language sql
as
$$
select 
    string_agg(format('%1$I = $%2$s :: %3$I', a.attname, pa.idx, t.typname), ', ' order by pa.idx)
from 
    pg_attribute a
    join pg_type t on a.atttypid = t.oid
    join unnest(field_names) with ordinality as pa (name, idx) on pa.name = a.attname
where 
    a.attrelid = relation
    and a.attname = any (field_names)
    and a.attnum > 0
    and not a.attisdropped;
$$;

/*
 * Given an array of argument names and a jsonb object in the shape of {[argument_name]:argument_value},
 * it returns a jsonb array with the argument values in the same order as the array inn field_names.
 * This can be used with postgrest_update_fields to build a SQL update command
 */ 
create or replace function _postgrest_update_ordered_field_values(field_names text[], passed_values jsonb)
    returns jsonb
    immutable
    language sql
as
$$
select 
    jsonb_agg(passed_values ->> (name::text) order by idx)
from 
     unnest(field_names) with ordinality as _ (name, idx);
$$;


create procedure postgrest_update (request omni_httpd.http_request, outcome inout omni_httpd.http_outcome, settings postgrest_settings default postgrest_settings ())
language plpgsql
as $$
declare
    namespace text;
    query text;
    result jsonb;
    relation regclass;
    preference text;
    _missing text;
    _return text := 'minimal';
    _tx text;
    payload jsonb;
begin
    if outcome is distinct from null then
        return;
    end if;
    if request.method = 'PATCH' then
        call omni_rest._postgrest_relation (request, relation, namespace, settings);
        if relation is null then
            return;
            -- terminate
        end if;
    else
        return;
        -- terminate;
    end if;

    -- change this when we implement other content types
    if omni_http.http_header_get (request.headers, 'content-type') <> 'application/json' then
        outcome := omni_httpd.http_response (status => 501, body => 'Only JSON content type is currently implemented');
        return;
    end if;

    -- while we support only json content type we can check its shape here
    payload := convert_from(request.body, 'utf8')::jsonb;
    if jsonb_typeof(payload) <> 'object' then
        outcome := omni_httpd.http_response (status => 422, body => 'Body must be object');
        return;
    end if;

    for preference in
        select regexp_split_to_table(omni_http.http_header_get (request.headers, 'prefer'), ',\s+')
    loop
        declare 
            preference_name text := split_part(preference, '=', 1);
            preference_value text := split_part(preference, '=', 2);
        begin
            case when preference_name = 'missing' then
                _missing := preference_value;
            when preference_name = 'return' then
                _return := preference_value;
            when preference_name = 'tx' then
                _tx := preference_value;
            end case;
        end;
    end loop;
    declare
        update_fields_definition text;
        field_values             jsonb;
        field_names              text[];
        params                   text[];
        _where                   text;
        _select   text;
        query_columns   text[];
    begin
        select 
            array_agg(jsonb_object_keys)
        from
            jsonb_object_keys(payload)
        into field_names;
        update_fields_definition :=
            coalesce(omni_rest._postgrest_update_fields(relation, field_names), '');
        field_values := omni_rest._postgrest_update_ordered_field_values(field_names, payload);
        params := omni_web.parse_query_string(request.query_string);
        _where := omni_rest.postgrest_format_get_param(omni_rest.postgrest_parse_get_param(params));
        -- Columns (vertical filtering)
        _select = omni_web.param_get(params, 'select');
        query_columns := omni_rest.postgrest_select_columns(namespace, relation, _select);

        query := 
            format('update %1$I.%2$I set %3$s where %4$s %5$s', 
                namespace, 
                (select
                    relname
                from pg_class
                where
                    oid = relation), 
                update_fields_definition,
                coalesce(_where, 'true'),
                case when _return = 'representation' then
                    format('returning %1$s', concat_ws(', ', variadic query_columns))
                else
                    ''
                end
            );

        select
            jsonb_agg(stmt_row)
        from
            omni_sql.execute (query, coalesce(field_values, '[]'::jsonb))
        into result;
        if lower(_tx) = 'rollback' then
            rollback and chain;
        end if;
        outcome := omni_httpd.http_response (
            status => 201, body => case when _return = 'representation' then result end
        );
    end;
end;
$$;


