create or replace function _camel_to_snake_case(input_text text)
    returns text as
$$
begin
    return lower(
            regexp_replace(
                    regexp_replace(input_text, '([a-z])([A-Z])', '\1_\2', 'g'),
                    '([A-Z])([A-Z][a-z])', '\1_\2', 'g'
            )
           );
end;
$$ language plpgsql immutable;

create or replace function _infer_plural_snake_case(plural_name text, kind text)
    returns text as
$$
declare
    kind_snake             text;
    inferred_plural        text;
    expected_no_underscore text;
begin
    kind_snake := _camel_to_snake_case(kind);

    -- Check if kind is already plural (matches the plural_name when underscores removed)
    if lower(plural_name) = replace(kind_snake, '_', '') then
        -- Kind is already plural, use as-is
        inferred_plural := kind_snake;
    else
        -- Apply pluralization rules
        if right(kind_snake, 1) = 'y' and not right(kind_snake, 2) in ('ay', 'ey', 'iy', 'oy', 'uy') then
            inferred_plural := left(kind_snake, length(kind_snake) - 1) || 'ies';
        elsif right(kind_snake, 1) in ('s', 'x', 'z') or right(kind_snake, 2) in ('ch', 'sh') then
            inferred_plural := kind_snake || 'es';
        else
            inferred_plural := kind_snake || 's';
        end if;
    end if;

    -- Validation: check if our inference matches the actual plural
    expected_no_underscore := replace(inferred_plural, '_', '');

    if lower(plural_name) != expected_no_underscore then
        raise warning 'Plural mismatch for %: expected % (becomes %), got %',
            kind, expected_no_underscore, inferred_plural, lower(plural_name);
    end if;

    return inferred_plural;
end;
$$ language plpgsql immutable;

create function resource_views(base_schema name,
                               group_versions text[] default null,
                               singular bool default true, underscore_case boolean default true,
                               schema_template text default '%s/%s')
    returns void
    language plpgsql
as
$resource_view$
declare
    ns              name := current_setting('omni_kube.search_path');
    old_search_path text := current_setting('search_path');
begin
    perform set_config('search_path', format('%I, public', ns), true);

    perform
        resource_view(case
                          when underscore_case then _camel_to_snake_case(preferred_name)
                          else preferred_name end,
                      g.name || '/' || g.preferred_version, r.name,
                      schema => case
                                    when g.name = 'core' then base_schema
                                    else format(schema_template, base_schema, g.name) end)
    from
        omni_kube.api_group                                                                  g
        inner join lateral (select *,
                                   (case
                                        when singular and underscore_case then kind
                                        when singular then singular_name
                                        when not singular and underscore_case then _infer_plural_snake_case(name, kind)
                                        else name end) as preferred_name
                            from
                                omni_kube.group_resources(name || '/' || preferred_version)) r on true

    where
        (group_versions is null or (g.name || '/' || g.preferred_version = any (group_versions))) and
        singular_name != '' and
        'list' = any (verbs);

    perform set_config('search_path', old_search_path, true);
end;
$resource_view$;
