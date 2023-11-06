create or replace function jsonb_set_at_path(target jsonb, path text[], new_value jsonb)
    returns jsonb as
$$
declare
    i            int;
    current_path text[];
    temp         jsonb;
    is_numeric   bool;
begin
    temp := target;

    for i in 1..array_length(path, 1)
        loop
            current_path := path[1:i];

            -- Check if the current path segment is numeric
            is_numeric := path[i] ~ E'^\\d+$';

            -- If the current path doesn't exist in temp
            if (temp #> current_path) is null then
                -- If the segment is numeric
                if is_numeric then
                    if ((temp #> current_path[1:i - 1]) = '{}'::jsonb) then
                        temp := jsonb_set(temp, current_path[1:i - 1], '[]');
                    end if;

                    -- If it's an array index, ensure the array has enough elements
                    if jsonb_typeof(temp #> current_path[1:i - 1]) = 'array' then
                        while jsonb_array_length(temp #> current_path[1:i - 1]) < path[i]::int
                            loop
                                temp := jsonb_set(temp, current_path[1:i - 1],
                                                  (temp #> current_path[1:i - 1]) || 'null'::jsonb);
                            end loop;
                        -- Set the value at the specific index
                        temp := jsonb_insert(temp, current_path, new_value);
                    end if;

                else
                    -- If it's not a numeric segment, ensure the path and then set the value
                    if i = array_length(path, 1) then
                        temp := jsonb_set(temp, current_path[1:i], new_value);
                    else
                        temp := jsonb_insert(temp, current_path[1:i], '{}'::jsonb);
                    end if;
                end if;
            else
                if is_numeric then
                    temp := jsonb_set(temp, current_path, new_value);
                end if;
            end if;
        end loop;

    return temp;
end;
$$ language plpgsql;

create function define_table_mapping(tab regclass, annotation jsonb default null) returns void
    language plpgsql
as
$$
declare
    map_code     text;
    col          record;
    build_pairs  text[] = '{}';
    ensure_paths text[] = '{}';
    i            int;
begin
    if annotation is null then
        annotation := '{}';
    end if;
    -- to_json[b]
    for col in select attname as name from pg_attribute where attrelid = tab and attnum > 0 and not attisdropped
        loop
            declare
                colannotation jsonb;
                name          text := col.name;
                output        jsonb;
                val           text;
            begin
                colannotation := annotation -> 'columns' -> name;

                val := 'rec.' || quote_ident(col.name);
                if colannotation is not null then
                    if colannotation -> 'exclude' is not null and
                       jsonb_typeof(colannotation -> 'exclude') = 'boolean' and
                       (colannotation -> 'exclude')::bool then
                        continue;
                    end if;

                    output := colannotation -> 'transform' -> 'output';

                    if output is not null and output -> 'function' is not null then
                        val := case
                                   when output ->> 'type' = 'json' then
                                       format('%s(%s)::jsonb', output ->> 'function', val)
                                   when output ->> 'type' = 'text' then
                                       format('%s(%s)::text', output ->> 'function', val)
                                   else
                                       format('%s(%s)', output ->> 'function', val)
                            end;
                    end if;

                    if colannotation -> 'path' is not null and (jsonb_typeof(colannotation -> 'path') = 'array') then
                        declare
                            path text[] :=
                                array(select jsonb_array_elements_text(colannotation -> 'path'));
                        begin
                            ensure_paths :=
                                        array [quote_literal(path::text), val] ||
                                        ensure_paths;
                            -- Don't build pairs
                            continue;
                        end;
                    end if;
                    if colannotation -> 'path' is not null and (jsonb_typeof(colannotation -> 'path') = 'string') then
                        name := colannotation ->> 'path';
                    end if;
                end if;
                build_pairs := build_pairs || array [quote_literal(name), val];
            end;
        end loop;

    map_code := format('jsonb_build_object(%s)', array_to_string(build_pairs, ','));

    for i in 1..coalesce(array_length(ensure_paths, 1), 0) by 2
        loop
            map_code :=
                    format('omni_json.jsonb_set_at_path(%s, %s, to_jsonb(%s))', map_code,
                           ensure_paths[i],
                           ensure_paths[i + 1]);
        end loop;

    map_code := format('select %s', map_code);

    execute format('
      create or replace function to_jsonb(rec %1$s) returns jsonb language sql as
      %2$L
    ', tab, map_code);

    execute format('
      create or replace function to_json(rec %1$s) returns json language sql as
      %2$L
    ', tab, 'select to_jsonb(rec)::json');

    -- json[b]_populate_record

    map_code := '';

    for i in 1..coalesce(array_length(build_pairs, 1), 0) by 2
        loop
            declare
                tmp   text;
                tmp1  text;
                input jsonb;
            begin
                tmp := build_pairs[i + 1];
                tmp1 := format('from_json->%s', build_pairs[i]);
                input := annotation -> 'columns' -> substring(tmp from 5) -> 'transform' -> 'input';
                if input is not null and input -> 'function' is not null then
                    tmp1 := case
                                when input ->> 'type' = 'json' then
                                    format('to_jsonb(%s((%s)::json))', input ->> 'function', tmp1)
                                when input ->> 'type' = 'text' then
                                    format('to_jsonb(%s((%s)::text))', input ->> 'function', tmp1)
                                else
                                    format('to_jsonb(%s(%s))', input ->> 'function', tmp1)
                        end;
                end if;
                build_pairs[i + 1] := format('coalesce(%1$s, to_jsonb(%2$s))', tmp1, tmp);
                build_pairs[i] := quote_literal(substring(tmp from 5));
            end;
        end loop;

    for i in 1..coalesce(array_length(ensure_paths, 1), 0) by 2
        loop
            declare
                tmp   text;
                tmp1  text;
                input jsonb;
            begin
                tmp := substring(ensure_paths[i + 1] from 5);
                tmp1 := format('from_json#>%s', ensure_paths[i]);
                input := annotation -> 'columns' -> tmp -> 'transform' -> 'input';
                if input is not null and input -> 'function' is not null then
                    tmp1 := case
                                when input ->> 'type' = 'json' then
                                    format('to_jsonb(%s((%s)::json))', input ->> 'function', tmp1)
                                when input ->> 'type' = 'text' then
                                    format('to_jsonb(%s((%s)::text))', input ->> 'function', tmp1)
                                else
                                    format('to_jsonb(%s(%s))', input ->> 'function', tmp1)
                        end;
                end if;
                build_pairs := build_pairs ||
                               array [quote_literal(tmp), format(
                                       'coalesce(%1$s, to_jsonb(%2$s))', tmp1, ensure_paths[i + 1])];
            end;
        end loop;

    -- Built object
    map_code := format('jsonb_build_object(%s)',
                       array_to_string(build_pairs, ','));


    map_code := format('select pg_catalog.jsonb_populate_record(rec::record, %s)',
                       map_code);

    execute format('
      create or replace function jsonb_populate_record(rec %1$s, from_json jsonb) returns %1$s language sql as
      %2$L
    ', tab, map_code);

    execute format('
      create or replace function json_populate_record(rec %1$s, from_json json) returns %1$s language sql as
      %2$L
    ', tab, 'select jsonb_populate_record(rec, from_json::jsonb)');

end
$$;