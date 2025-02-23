/*
This code is partially based on meta from the Aquameta project (https://github.com/aquameta/meta),
licensed under the terms of BSD-2 Clause License. It started off as an integration but became a
significant re-working.
*/

-- for some reason, generator isn't generating these.  they're commented out.
create function field_id_to_row_id(field_id field_id) returns row_id as $_$ select row_id((field_id).schema_name, (field_id).relation_name, (field_id).pk_column_names, (field_id).pk_values) $_$ immutable language sql;
create cast (field_id as row_id) with function field_id_to_row_id(field_id) as assignment;


------------------------------------------------------------------------
-- helpers
-- non-standard constructors that do sensible things
------------------------------------------------------------------------

-- field_id constructor taking a row_id
create function field_id(row_id row_id, column_name name) returns field_id as $$
    select field_id(row_id.schema_name, row_id.relation_name, row_id.pk_column_names, row_id.pk_values, column_name);
$$ language sql;


-- field_id constructor non-array pk
create function field_id( schema_name name, relation_name name, pk_column_name name, pk_value name, column_name name) returns field_id as $$
    select field_id(schema_name, relation_name, array[pk_column_name], array[pk_value], column_name);
$$ language sql;


-- single key row_id constructor
create function row_id(schema_name text, relation_name name, pk_column_name name, pk_value name) returns row_id as $_$ select row_id(schema_name, relation_name, array[pk_column_name], array[pk_value]) $_$ immutable language sql;


------------------------------------------------------------------------
-- pk_stmt()
-- helper function for iterating primary key arrays and generating a stmt fragment.
------------------------------------------------------------------------

-- template is rendered by format(), using positional argument notation.
--     1: pk_column_names[i]
--     2: pk_values[i]
--     3: i
--
-- select _pk_stmt (
--     array['id','other_id'],
--     array[public.uuid_generate_v4()::text,public.uuid_generate_v4()::text],
--     '(row_id).pk_values[%3$s] = x.%1$I'
-- );
--                               _pk_stmt                               
-- ---------------------------------------------------------------------
--  (row_id).pk_values[1] = x.id and (row_id).pk_values[2] = x.other_id

create or replace function _pk_stmt(pk_column_names text[], pk_values text[], template text, delimeter text default ' and ') returns text as $$
    declare
        pk_comparisons text[];
        column_name text;
        i integer;
    begin
        i := 1;
        foreach column_name in array pk_column_names loop
            pk_comparisons[i] := format(template, pk_column_names[i], pk_values[i], i);
            i := i + 1;
        end loop;
        return array_to_string(pk_comparisons, delimeter);
    end
$$ language plpgsql;

/*
select _pk_stmt (
    row_id(
        'public',
        'foo',
        array['id','other_id'],
        array[public.uuid_generate_v4()::text,public.uuid_generate_v4()::text]
    ),
    '(row_id).pk_values[%3$s] = x.%1$I',
    ' OR '
);
                              _pk_stmt                              
--------------------------------------------------------------------
 (row_id).pk_values[1] = x.id OR (row_id).pk_values[2] = x.other_id
*/

create function _pk_stmt(row_id row_id, template text, delimeter text default ' and ') returns text as $$
    select _pk_stmt((row_id).pk_column_names, (row_id).pk_values, template, delimeter);
$$ language sql;


create or replace function field_id_literal_value(field_id field_id) returns text as $$
declare
    literal_value text;
    stmt text;
    pk_stmt text;
begin
    pk_stmt := _pk_stmt (
        (field_id).pk_column_names,
        (field_id).pk_values,
        '%1$I = %2$L'
    );

    stmt := format('select %I::text from %I.%I where %s',
        (field_id).column_name,
        (field_id).schema_name,
        (field_id).relation_name,
        pk_stmt
    );
    -- raise notice 'stmt: %', stmt;

    execute stmt into literal_value;

    return literal_value;
-- TODO: is this correct?  this fires when the table doesn't exist etc.
exception when others then
    raise warning 'field_id_literal_value exception on %: %', field_id, SQLERRM;
    return null;
end
$$ language plpgsql stable;



create or replace function row_exists(in row_id row_id, out answer boolean) as $$
    declare
        stmt text;
        pk_comparisons text[];
        pk_comparison_stmt text;
        column_name text;
        i integer;
    begin
        -- generate the pk comparisons line
        i := 1;
        foreach column_name in array row_id.pk_column_names loop
            pk_comparisons[i] := quote_ident((row_id).pk_column_names[i]) || '::text = ' || quote_literal((row_id).pk_values[i]);
            i := i + 1;
        end loop;
        pk_comparison_stmt := array_to_string(pk_comparisons, ' and ');


        stmt := format (
            -- 'select (count(*) = 1) from %I.%I where %I::text = %L',
            'select (count(*) = 1) from %I.%I where %s',
                (row_id).schema_name,
                (row_id).relation_name,
                pk_comparison_stmt
            );

        raise debug '%', stmt;
        execute stmt into answer;

    exception
        when undefined_table then
            answer := false;
    end;
$$ language plpgsql;
