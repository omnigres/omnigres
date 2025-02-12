/*
This code is based on meta from the Aquameta project (https://github.com/aquameta/meta):

BSD 2-Clause License

Copyright (c) 2019, Eric Hanson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

-- returns an array of types, given an identity_args string (as provided by pg_get_function_identity_arguments())
create or replace function _get_function_type_sig_array(identity_args text) returns text[] as $$
declare
    param_exprs text[] := '{}';
    param_expr text[] := '{}';
    sig_array text[] := '{}';
    len integer;
    len2 integer;
    cast_try text;
    good_type text;
    good boolean;
begin
    -- raise notice '# type_sig_array got: %', identity_args;
    param_exprs := split_quoted_string(identity_args,',');
    len := array_length(param_exprs,1);
    -- raise notice '# param_exprs: %, length: %', param_exprs, len;
    if len is null or len = 0 or param_exprs[1] = '' then
        -- raise notice '    NO PARAMS';
        return '{}'::text[];
    end if;
    -- raise notice 'type_sig_array after splitting into individual exprs (length %): %', len, param_exprs;

    -- for each parameter expression
    for i in 1..len
        loop
            good_type := null;
            -- split by spaces (but not spaces within quotes)
            param_expr = split_quoted_string(param_exprs[i], ' ');
            -- raise notice '        type_sig_array expr: %, length is %', param_expr, len2;

            -- skip OUTs for type-sig, the function isn't called with those
            continue when param_expr[1] = 'OUT';

            len2 := array_length(param_expr,1);
            if len2 is null then
                raise warning 'len2 is null, i: %, identity_args: %, param_exprs: %, param_expr: %', i, identity_args, param_exprs, param_expr;
            end if;

            -- no params;
            continue when len2 is null;

            -- ERROR:  len2 is null, i: 1, identity_args: "char", name, name, name[]
            for j in 1..len2 loop
                    cast_try := array_to_string(param_expr[j:],' ');
                    -- raise notice '    !!! casting % to ::regtype', cast_try;
                    begin
                        execute format('select %L::regtype', cast_try) into good_type;
                    exception when others then
                    -- raise notice '        couldnt cast %', cast_try;
                    end;

                    if good_type is not null then
                        -- raise notice '    GOT A TYPE!! %', good_type;
                        sig_array := array_append(sig_array, good_type);
                        exit;
                    else
                        -- raise notice '    Fail.';
                    end if;
                end loop;

            if good_type = null then
                raise exception 'Could not parse function parameter: %', param_expr;
            end if;
        end loop;
    return sig_array;
end
$$ language plpgsql stable;

/******************************************************************************
 * siuda
 *****************************************************************************/
create type siuda as enum ('select', 'insert', 'update', 'delete', 'all');

create function siuda(c char) returns siuda as $$
begin
    case c
        when 'r' then
            return 'select'::siuda;
        when 'a' then
            return 'insert'::siuda;
        when 'w' then
            return 'update'::siuda;
        when 'd' then
            return 'delete'::siuda;
        when '*' then
            return 'all'::siuda;
    end case;
end;
$$ immutable language plpgsql;


create cast (char as siuda)
with function siuda(char)
as assignment;


/******************************************************************************
 * schema
 *****************************************************************************/
create view schema as
    select schema_id(schema_name) id, schema_name::text as name
    from information_schema.schemata
--    where schema_name not in ('pg_catalog', 'information_schema')
;


/******************************************************************************
 * type
 *****************************************************************************/

create or replace view type as
select type_id(n.nspname, t.typname::text) as id,
    n.nspname::text as schema_name,
       t.typname                           as name
from pg_catalog.pg_type t
     left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
     left join pg_catalog.pg_class c on c.oid = t.typrelid
where (t.typrelid = 0 or c.relkind = 'c')
  and not exists(select 1 from pg_catalog.pg_type el where el.oid = t.typelem and el.typarray = t.oid)
--   and pg_catalog.pg_type_is_visible(t.oid)
    AND n.nspname <> 'information_schema'
;


create or replace view type_basic as
select type_id(n.nspname, t.typname::text) as id
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where typtype = 'b';

create or replace view type_composite as
select type_id(n.nspname, t.typname::text) as id
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where typtype = 'c';

create or replace view type_composite_attribute as
select type_id(n.nspname, t.typname::text) as id,
        a.attname as name
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
         join pg_attribute a on a.attrelid=t.typrelid
where t.typtype = 'c'
        and a.attnum > 0
        and not a.attisdropped;

create or replace view type_composite_attribute_position as
    select type_id(n.nspname, t.typname::text) as id,
           a.attname as name,
           a.attnum as position
    from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
         join pg_attribute a on a.attrelid=t.typrelid
    where t.typtype = 'c'
            and a.attnum > 0
            and not a.attisdropped;

create or replace view type_composite_attribute_collation as
    select type_id(n.nspname, t.typname::text) as id,
           a.attname as name,
           co.collname as collation
    from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
         join pg_attribute a on a.attrelid=t.typrelid
         join pg_type ct on ct.oid=a.atttypid
         left join pg_collation co on co.oid=a.attcollation
    where t.typtype = 'c'
            and a.attnum > 0
            and not a.attisdropped
    and a.attcollation <> ct.typcollation;

create or replace view type_domain as
select type_id(n.nspname, t.typname::text)   as id,
       type_id(n1.nspname, t1.typname::text) as base_type_id
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
         inner join pg_catalog.pg_type t1 on t1.oid = t.typbasetype
         left join pg_catalog.pg_namespace n1 on n1.oid = t1.typnamespace
where t.typtype = 'd';

create or replace view type_enum as
select type_id(n.nspname, t.typname::text) as id
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where typtype = 'e';

create or replace view type_enum_label as
select type_id(n.nspname, t.typname::text) as id,
       enumlabel                           as label,
       enumsortorder                       as sortorder
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
         inner join pg_enum e on e.enumtypid = t.oid
where typtype = 'e';

create or replace view type_pseudo as
select type_id(n.nspname, t.typname::text) as id
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where typtype = 'p';

create or replace view type_range as
select type_id(n.nspname, t.typname::text) as id
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where typtype = 'r';

create or replace view type_multirange as
select type_id(n.nspname, t.typname::text) as id
from pg_catalog.pg_type t
         left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
where typtype = 'm';



/******************************************************************************
 * cast
 *****************************************************************************/
create view "cast" as
    select
        cast_id(ts.typname, pg_catalog.format_type(castsource, null), tt.typname,
                pg_catalog.format_type(casttarget, null)) as id,
        pg_catalog.format_type(castsource, null)          as "from",
        pg_catalog.format_type(casttarget, null)          as "to"
    from
        pg_catalog.pg_cast                c
        left join pg_catalog.pg_type      ts
                  on c.castsource = ts.oid
        left join pg_catalog.pg_namespace ns
                  on ns.oid = ts.typnamespace
        left join pg_catalog.pg_type      tt
                  on c.casttarget = tt.oid;

create view cast_binary_coercible as
    select
        cast_id(ts.typname, pg_catalog.format_type(castsource, null), tt.typname,
                pg_catalog.format_type(casttarget, null)) as id
    from
        pg_catalog.pg_cast                c
        inner join pg_catalog.pg_type      ts
                  on c.castsource = ts.oid
        inner join pg_catalog.pg_namespace ns
                  on ns.oid = ts.typnamespace
        inner join pg_catalog.pg_type      tt
                  on c.casttarget = tt.oid
    where
        castfunc = 0;

create view cast_function as
    select
        cast_id(ts.typname, pg_catalog.format_type(castsource, null), tt.typname,
                pg_catalog.format_type(casttarget, null)) as id,
        function_id(
            pns.nspname,
            p.proname,
            _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid))
        )
    from
        pg_catalog.pg_cast                 c
        inner join pg_catalog.pg_proc      p
                   on c.castfunc = p.oid
        inner join pg_catalog.pg_namespace pns
                   on pns.oid = p.pronamespace
        inner join  pg_catalog.pg_type      ts
                   on c.castsource = ts.oid
        inner join  pg_catalog.pg_namespace ns
                   on ns.oid = ts.typnamespace
        inner join  pg_catalog.pg_type      tt
                   on c.casttarget = tt.oid
where castfunc != 0;

create view cast_implicit_in_assignment as
    select
        cast_id(ts.typname, pg_catalog.format_type(castsource, null), tt.typname,
                pg_catalog.format_type(casttarget, null)) as id
    from
        pg_catalog.pg_cast                 c
        inner join  pg_catalog.pg_type      ts
                    on c.castsource = ts.oid
        inner join  pg_catalog.pg_namespace ns
                    on ns.oid = ts.typnamespace
        inner join  pg_catalog.pg_type      tt
                    on c.casttarget = tt.oid
where castcontext = 'a';

create view cast_explicit as
    select
        cast_id(ts.typname, pg_catalog.format_type(castsource, null), tt.typname,
                pg_catalog.format_type(casttarget, null)) as id
    from
        pg_catalog.pg_cast                 c
        inner join  pg_catalog.pg_type      ts
                    on c.castsource = ts.oid
        inner join  pg_catalog.pg_namespace ns
                    on ns.oid = ts.typnamespace
        inner join  pg_catalog.pg_type      tt
                    on c.casttarget = tt.oid
    where castcontext = 'e';

create view cast_implicit as
    select
        cast_id(ts.typname, pg_catalog.format_type(castsource, null), tt.typname,
                pg_catalog.format_type(casttarget, null)) as id
    from
        pg_catalog.pg_cast                 c
        inner join  pg_catalog.pg_type      ts
                    on c.castsource = ts.oid
        inner join  pg_catalog.pg_namespace ns
                    on ns.oid = ts.typnamespace
        inner join  pg_catalog.pg_type      tt
                    on c.casttarget = tt.oid
    where castcontext = 'i';

/******************************************************************************
 * operator
 *****************************************************************************/
create view "operator" as
SELECT operator_id(n.nspname, o.oprname, trns.nspname, tr.typname, trns.nspname, tr.typname) as id,
    n.nspname::text as schema_name,
    o.oprname::text as name,
    CASE WHEN o.oprkind='l' THEN NULL ELSE pg_catalog.format_type(o.oprleft, NULL) END AS "Left arg type",
    CASE WHEN o.oprkind='r' THEN NULL ELSE pg_catalog.format_type(o.oprright, NULL) END AS "Right arg type",
    pg_catalog.format_type(o.oprresult, NULL) AS "Result type",
    coalesce(pg_catalog.obj_description(o.oid, 'pg_operator'),
        pg_catalog.obj_description(o.oprcode, 'pg_proc')) AS "Description"
FROM pg_catalog.pg_operator o
    LEFT JOIN pg_catalog.pg_namespace n ON n.oid = o.oprnamespace
    JOIN pg_catalog.pg_type tl ON o.oprleft = tl.oid
    JOIN pg_catalog.pg_namespace tlns on tl.typnamespace = tlns.oid
    JOIN pg_catalog.pg_type tr ON o.oprleft = tr.oid
    JOIN pg_catalog.pg_namespace trns on tr.typnamespace = trns.oid
WHERE n.nspname <> 'pg_catalog'
    AND n.nspname <> 'information_schema'
--    AND pg_catalog.pg_operator_is_visible(o.oid)
-- ORDER BY 1, 2, 3, 4;
;


/******************************************************************************
 * sequence
 *****************************************************************************/
create view "sequence" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           ns.nspname as schema_name,
           c.relname::text as name
    from pg_sequence s
inner join pg_class c on c.oid = s.seqrelid
inner join pg_namespace ns on ns.oid = c.relnamespace;

create view "sequence_start_value" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           s.seqstart as start_value
    from pg_sequence s
         inner join pg_class c on c.oid = s.seqrelid
         inner join pg_namespace ns on ns.oid = c.relnamespace;

create view "sequence_minimum_value" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           s.seqmin as minimum_value
    from pg_sequence s
         inner join pg_class c on c.oid = s.seqrelid
         inner join pg_namespace ns on ns.oid = c.relnamespace;

create view "sequence_maximum_value" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           s.seqmax as maximum_value
    from pg_sequence s
         inner join pg_class c on c.oid = s.seqrelid
         inner join pg_namespace ns on ns.oid = c.relnamespace;

create view "sequence_increment" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           s.seqincrement as increment
    from pg_sequence s
         inner join pg_class c on c.oid = s.seqrelid
         inner join pg_namespace ns on ns.oid = c.relnamespace;

create view "sequence_cache" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           s.seqcache as cache
    from pg_sequence s
         inner join pg_class c on c.oid = s.seqrelid
         inner join pg_namespace ns on ns.oid = c.relnamespace;

create view "sequence_cycle" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id
    from pg_sequence s
         inner join pg_class c on c.oid = s.seqrelid
         inner join pg_namespace ns on ns.oid = c.relnamespace
    where s.seqcycle;

create view "sequence_type" as
    select sequence_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           type_id(tns.nspname, t.typname)
    from pg_sequence s
         inner join pg_class c on c.oid = s.seqrelid
         inner join pg_namespace ns on ns.oid = c.relnamespace
         inner join pg_type t on t.oid = s.seqtypid
         inner join pg_namespace tns on tns.oid = t.typnamespace;

create view "sequence_table" as
    select
        sequence_id(ns.nspname, c.relname) as id,
        schema_id(ns.nspname)              as schema_id,
        table_id(rns.nspname, r.relname)
    from
        pg_sequence             s
        inner join pg_class     c on c.oid = s.seqrelid
        inner join pg_namespace ns on ns.oid = c.relnamespace
        left join  pg_depend    d
                   on d.objid = c.oid
                       and d.classid = 'pg_class'::regclass
                       and d.deptype in ('a', 'i')
        left join pg_class r on r.oid = d.refobjid
        left join pg_namespace rns on rns.oid = r.relnamespace
    where
        r is distinct from null and
        c.relkind = 'S';

/******************************************************************************
 * table
 *****************************************************************************/
create view "table" as
    select relation_id(schemaname, tablename) as id,
           schema_id(schemaname) as schema_id,
           schemaname::text as schema_name,
           tablename::text as name
    from pg_catalog.pg_tables;

create view table_rowsecurity as
    select relation_id(schemaname, tablename) as id
    from pg_catalog.pg_tables
where pg_tables.rowsecurity;

create view table_forcerowsecurity as
    select relation_id(schemaname, tablename) as id
    from pg_catalog.pg_tables
inner join pg_catalog.pg_namespace ns on pg_tables.schemaname = ns.nspname
inner join pg_catalog.pg_class c on pg_tables.tablename = c.relname and ns.oid = c.relnamespace
where relforcerowsecurity;

/******************************************************************************
 * view
 *****************************************************************************/
create view "view" as
    select relation_id(table_schema, table_name) as id,
           schema_id(table_schema) as schema_id,
           table_schema::text as schema_name,
           table_name::text as name
    from information_schema.views v;

create view view_definition as
    select relation_id(table_schema, table_name) as id,
           view_definition::text as query
    from information_schema.views v;

/******************************************************************************
 * relation_column
 *****************************************************************************/
create view relation_column as
    select column_id(c.table_schema, c.table_name, c.column_name) as id,
           relation_id(c.table_schema, c.table_name) as relation_id,
           c.column_name::text as name
    from information_schema.columns c;

create view relation_column_position as
select column_id(c.table_schema, c.table_name, c.column_name) as id,
       c.ordinal_position::integer                            as position
from information_schema.columns c;

create view relation_column_default as
    select column_id(c.table_schema, c.table_name, c.column_name) as id,
           c.column_default::text as "default"
    from information_schema.columns c where c.column_default is not null;

create view relation_column_type as
    select column_id(c.table_schema, c.table_name, c.column_name) as id,
           type_id (c.udt_schema, c.udt_name) as "type_id"
    from information_schema.columns c;

create view relation_column_nullable as
    select column_id(c.table_schema, c.table_name, c.column_name) as id
    from information_schema.columns c
        where c.is_nullable = 'YES';

/******************************************************************************
 * relation
 *****************************************************************************/
create view relation as
    select relation_id(t.table_schema, t.table_name) as id,
           schema_id(t.table_schema) as schema_id,
           t.table_schema::text as schema_name,
           t.table_name::text as name
    from information_schema.tables t;

create view relation_primary_keys as
    select relation_id(t.table_schema, t.table_name) as id,
           column_id(c.table_schema, c.table_name, k.column_name),
           k.ordinal_position as position
    from information_schema.tables t
         inner join information_schema.table_constraints c
                   on t.table_catalog = c.table_catalog and
                      t.table_schema = c.table_schema and
                      t.table_name = c.table_name and
                      c.constraint_type = 'PRIMARY KEY'
         inner join information_schema.key_column_usage k
                   on k.constraint_catalog = c.constraint_catalog and
                      k.constraint_schema = c.constraint_schema and
                      k.constraint_name = c.constraint_name;

/******************************************************************************
 * foreign_key
 *****************************************************************************/
create or replace view foreign_key as
select constraint_id(from_schema_name, from_table_name, constraint_name) as id,
    from_schema_name::text as schema_name,
    from_table_name::text as table_name,
    constraint_name::text,
    array_agg(from_column_name::text order by from_col_key_position) as from_column_names,
    to_schema_name::text,
    to_table_name::text,
    array_agg(to_column_name::text order by to_col_key_position) as to_column_names,
    match_option::text,
    on_update::text,
    on_delete::text
from (
    select
        ns.nspname as from_schema_name,
        cl.relname as from_table_name,
        c.conname as constraint_name,
        a.attname as from_column_name,
        from_cols.elem as from_column_num,
        from_cols.nr as from_col_key_position,
        to_ns.nspname as to_schema_name,
        to_cl.relname as to_table_name,
        to_a.attname as to_column_name,
        to_cols.elem as to_column_num,
        to_cols.nr as to_col_key_position,

/* big gank from information_schema.referential_constraints view */
        CASE c.confmatchtype
            WHEN 'f'::"char" THEN 'FULL'::text
            WHEN 'p'::"char" THEN 'PARTIAL'::text
            WHEN 's'::"char" THEN 'SIMPLE'::text -- was 'NONE'
            ELSE NULL::text
        END::information_schema.character_data AS match_option,
        CASE c.confupdtype
            WHEN 'c'::"char" THEN 'CASCADE'::text
            WHEN 'n'::"char" THEN 'SET NULL'::text
            WHEN 'd'::"char" THEN 'SET DEFAULT'::text
            WHEN 'r'::"char" THEN 'RESTRICT'::text
            WHEN 'a'::"char" THEN 'NO ACTION'::text
            ELSE NULL::text
        END::information_schema.character_data AS on_update,
        CASE c.confdeltype
            WHEN 'c'::"char" THEN 'CASCADE'::text
            WHEN 'n'::"char" THEN 'SET NULL'::text
            WHEN 'd'::"char" THEN 'SET DEFAULT'::text
            WHEN 'r'::"char" THEN 'RESTRICT'::text
            WHEN 'a'::"char" THEN 'NO ACTION'::text
            ELSE NULL::text
        END::information_schema.character_data AS on_delete
/* end big gank */

    from pg_constraint c
    join lateral unnest(c.conkey) with ordinality as from_cols(elem, nr) on true
    join lateral unnest(c.confkey) with ordinality as to_cols(elem, nr) on to_cols.nr = from_cols.nr -- FTW!
    join pg_namespace ns on ns.oid = c.connamespace
    join pg_class cl on cl.oid = c.conrelid
    join pg_attribute a on a.attrelid = c.conrelid and a.attnum = from_cols.elem

    -- to_cols
    join pg_class to_cl on to_cl.oid = c.confrelid
    join pg_namespace to_ns on to_cl.relnamespace = to_ns.oid
    join pg_attribute to_a on to_a.attrelid = to_cl.oid and to_a.attnum = to_cols.elem

    where contype = 'f'
) c_cols
group by 1,2,3,4,6,7,9,10,11;



/******************************************************************************
 * function
 *****************************************************************************/

-- splits a string of identifiers, some of which are quoted, based on the provided delimeter, but not if it is in quotes.
create or replace function split_quoted_string(input_str text, split_char text) returns text[] as $$
declare
    result_array text[];
    inside_quotes boolean := false;
    current_element text := '';
    char_at_index text;
begin
    for i in 1 .. length(input_str) loop
        char_at_index := substring(input_str from i for 1);

        -- raise notice 'current_element: __%__, char_at_index: __%__, inside_quotes: __%__', current_element, char_at_index, inside_quotes;
        -- we got a quote
        if char_at_index = '"' then
            -- raise notice '    ! got quote';
            -- is it a double double-quote?
            if substring(input_str from i + 1 for 1) = '"' then
                -- yes!
                -- raise notice '        a double double quote!';
                current_element := current_element || '""';
                i := i + 1; -- skip the next quote
            else
                -- no.  single quote means flip inside_quotes.
                -- raise notice '        a mere single double-quote';
                inside_quotes := not inside_quotes;
                current_element := current_element || '"';
            end if;
        -- non-quote char
        else
            -- is it the infamous split_charn
            if char_at_index = split_char and not inside_quotes then
                -- raise notice '    ! got the split char __%__ outside quotes', split_char;
                -- add current_element to the results_array
                result_array := array_append(result_array, trim(current_element));
                -- clear current_element
                current_element := '';
            -- no, just a normal char
            else
                -- raise notice '    . normal char, adding __%__', char_at_index;
                current_element := current_element || char_at_index;
                -- raise notice '    current_element is now__%__', current_element;
            end if;
        end if;

        -- if this is the last character, add current_element
        if i = length(input_str) then
            result_array := array_append(result_array, trim(current_element));
        end if;

    end loop;

    return result_array;
end;
$$ language plpgsql stable;

create or replace function _get_function_parameters(parameters text) returns text[] as $$
    declare
        param_exprs text[] := '{}';
        param_expr text[] := '{}';
        result text[] := '{}';
        default_pos integer;
        params_len integer;
        param_len integer;
    begin
        -- raise notice 'get_function_parameters got: %', parameters;
        param_exprs := split_quoted_string(parameters, ',');

        params_len := array_length(param_exprs,1);
        if params_len is null or params_len = 0 or param_exprs[1] = '' then
            -- raise notice '   NO PARAMS';
            return '{}'::text[];
        end if;

        -- raise notice 'get_function_parameters after splitting into individual exprs: %', param_exprs;
        -- for each parameter, drop OUTs, slice off INOUTs and trim everything past 'DEFAULT'
        for i in 1..params_len loop
            -- split by spaces (but not spaces within quotes)
            param_expr = split_quoted_string(param_exprs[i],' ');
            param_len := array_length(param_expr,1);
            -- raise notice '    get_function_parameters expr: %, length is %', param_expr, array_length(param_expr,1);

            result := array_append(result, param_exprs[i]);
        end loop;
        return result;
    end
$$ language plpgsql stable;

create or replace function _pg_get_function_sqlbody(p pg_proc) returns text language plpgsql stable
as $$
begin
    if (current_setting('server_version_num')::int/10000) = 13 then
        return p.prosrc;
    else
        return coalesce(pg_catalog.pg_get_function_sqlbody(p.oid), p.prosrc);
    end if;
end;
$$;

create or replace function _pg_get_function_result(p oid) returns text language sql stable
as $$ select pg_get_function_result(p); $$
    -- Force qualified naming
    set search_path = '';


create or replace view "callable" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace)
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id, -- function_id
        schema_id(schema_name) as schema_id,
        schema_name,
        name,
        type_sig
    from
        orig;


create or replace view "callable_function" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.prokind = 'f')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_argument_name" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                argname, ordinality
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
                inner join lateral (select * from unnest(proargnames) with ordinality as a(argname, ordinality)) as t(argname, ordinality) on true
           )
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        argname as argument_name,
        ordinality as argument_position
    from
        orig where argname != '';

create or replace view "callable_argument_type" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                argtype, ordinality
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
                inner join lateral (select * from unnest(coalesce(proallargtypes, proargtypes)) with ordinality as a(argtype, ordinality)) as t(argtype, ordinality) on true
        )
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        type_id(tns.nspname, t.typname) as type_id,
        ordinality as argument_position
    from orig
         inner join pg_type t on t.oid = argtype
         inner join pg_namespace tns on tns.oid = t.typnamespace;

create or replace view "callable_argument_mode" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                argmode, ordinality
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
                inner join lateral (select * from unnest(coalesce(proargmodes, array_fill('i'::"char", array[coalesce(cardinality(proallargtypes), pronargs)]))) with ordinality as a(argmode, ordinality)) as t(argmode, ordinality) on true
        )
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        ordinality as argument_position,
    case when argmode = 'i' then 'in'
         when argmode = 'o' then 'out'
         when argmode = 'b' then 'inout'
         when argmode = 'v' then 'variadic'
         when argmode = 't' then 'table'
    end as argument_mode
    from orig;


create or replace view "callable_argument_default" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                argdefault, ordinality
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
                inner join lateral (select pg_get_function_arg_default(p.oid, ordinality), ordinality from generate_series(1, p.pronargs) ordinality) as t(argdefault, ordinality) on true
        )
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        argdefault as argument_default,
        ordinality as argument_position
    from orig
    where argdefault is not null;

create or replace view "callable_aggregate" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.prokind = 'a')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_window" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.prokind = 'w')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_procedure" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.prokind = 'p')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_immutable" as
with
    orig as (
        select
            n.nspname                                                               as "schema_name",
            p.proname                                                               as "name",
            _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
        from
            pg_proc                            p
            inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
        where
            p.provolatile = 'i')
select
    function_id(
            schema_name,
            name,
            type_sig
    )                      as id
from
    orig;

create or replace view "callable_stable" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.provolatile = 's')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_volatile" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.provolatile = 'v')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_parallel_safe" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.proparallel = 's')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_parallel_restricted" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.proparallel = 'r')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_parallel_unsafe" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.proparallel = 'u')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_security_definer" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.prosecdef)
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_security_invoker" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                not p.prosecdef)
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id
    from
        orig;

create or replace view "callable_language" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                p.prolang
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                not p.prosecdef)
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        l.lanname as language
    from orig
         inner join pg_language l on l.oid = prolang;

create or replace view "callable_acl" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                p.proacl
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                not p.prosecdef)
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        aclitem as acl
    from orig
      inner join lateral (select * from unnest(proacl)) acl(aclitem) on true;

create or replace view "callable_owner" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                p.proowner
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                not p.prosecdef)
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        pg_get_userbyid(proowner) as owner
    from orig;

create or replace view "callable_body" as
    with
        orig as (
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                _pg_get_function_sqlbody(p) as body
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                not p.prosecdef)
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                      as id,
        body
    from orig;

create or replace view "callable_return_type" as
    with
        orig as (
            -- slightly modified version of query output by \df+
            select
                n.nspname                                                               as "schema_name",
                p.proname                                                               as "name",
                _get_function_type_sig_array(pg_get_function_identity_arguments(p.oid)) as "type_sig",
                _pg_get_function_result(p.oid)                                          as "return_type",
                p.prorettype                                                            as "return_type_id",
                p.proretset as "returns_set"
            from
                pg_proc                            p
                inner join pg_catalog.pg_namespace n on n.oid = p.pronamespace
            where
                p.prokind = 'f')
    select
        function_id(
                schema_name,
                name,
                type_sig
        )                                         as id,
        returns_set as setof,
        type_id(ns.nspname, t.typname) as return_type_id
    from
        orig
        inner join pg_type      t on t.oid = return_type_id
        inner join pg_namespace ns on ns.oid = t.typnamespace;

/******************************************************************************
 * trigger
 *****************************************************************************/
create view trigger as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id,
           t.id as relation_id,
           t_pgn.nspname::text as schema_name,
           pgc.relname::text as relation_name,
           pg_trigger.tgname::text as name,
--            f.id as function_id,
           case when (tgtype >> 1 & 1)::bool then 'before'
                when (tgtype >> 6 & 1)::bool then 'before'
                else 'after'
           end as "when",
           (tgtype >> 2 & 1)::bool as "insert",
           (tgtype >> 3 & 1)::bool as "delete",
           (tgtype >> 4 & 1)::bool as "update",
           (tgtype >> 5 & 1)::bool as "truncate",
           case when (tgtype & 1)::bool then 'row'
                else 'statement'
           end as level

    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace

    inner join "schema" t_s
            on t_s.name = t_pgn.nspname

    inner join "table" t
            on t.schema_id = t_s.id and
               t.name = pgc.relname

    inner join pg_proc pgp
            on pgp.oid = tgfoid

    inner join pg_namespace f_pgn
            on f_pgn.oid = pgp.pronamespace

    inner join "schema" f_s
            on f_s.name = f_pgn.nspname;


/******************************************************************************
 * role
 *****************************************************************************/
create view role as
   select role_id(pgr.rolname) as id,
          pgr.rolname::text  as name
   from pg_roles pgr
    union all
   select role_id('0'::oid::regrole::text) as id,
    'PUBLIC' as name;

create view role_superuser as
   select role_id(pgr.rolname) as id
   from pg_roles pgr
   where pgr.rolsuper;

create view role_inherit as
   select role_id(pgr.rolname) as id
   from pg_roles pgr
   where pgr.rolinherit;

create view role_create_role as
   select role_id(pgr.rolname) as id
   from pg_roles pgr
   where pgr.rolcreaterole;

create view role_create_db as
   select role_id(pgr.rolname) as id
   from pg_roles pgr
   where pgr.rolcreatedb;

create view role_can_login as
   select role_id(pgr.rolname) as id
   from pg_roles pgr
   where pgr.rolcanlogin;

create view role_replication as
   select role_id(pgr.rolname) as id
   from pg_roles pgr
   where pgr.rolreplication;

create view role_connection_limit as
   select role_id(pgr.rolname) as id,
          pgr.rolconnlimit   as connection_limit
   from pg_roles pgr;

create view role_valid_until as
   select role_id(pgr.rolname) as id,
          pgr.rolvaliduntil   as valid_until
   from pg_roles pgr;

create view role_setting as
    select role_setting_id(pgr.rolname, pgd.datname, split_part(setting, '=', 1)) as id,
           role_id(pgr.rolname) as role_id,
           split_part(setting, '=', 1) as setting_name,
           split_part(setting, '=', 2) as setting_value
    from pg_db_role_setting pgrs
        join pg_roles pgr on pgr.oid = pgrs.setrole
        left join pg_database pgd on pgd.oid = pgrs.setdatabase,
        unnest(pgrs.setconfig) as setting;

/******************************************************************************
 * role_inheritance
 *****************************************************************************/
create view role_inheritance as
select
    role_id(r.rolname::text) as id,
    r.rolname::text || '<-->' || r2.rolname::text as inheritance,
    r.rolname::text as role_name,
    role_id(r2.rolname::text) as member_role_id,
    r2.rolname::text as member_role_name
from pg_auth_members m
    join pg_roles r on r.oid = m.roleid
    join pg_roles r2 on r2.oid = m.member;



/******************************************************************************
 * table_privilege
 *****************************************************************************/
create view table_privilege as
select table_privilege_id(schema_name, table_name, (role_id).name, type) as id,
    relation_id(schema_name, table_name) as table_id,
    schema_name::text,
    table_name::text,
    role_id,
    (role_id).name as role_name,
    type::text,
    is_grantable::boolean,
    with_hierarchy::boolean
from (
    select
        case grantee
            when 'PUBLIC' then
                role_id('-'::text)
            else
                role_id(grantee::text)
        end as role_id,
        table_schema as schema_name,
        table_name,
        privilege_type as type,
        is_grantable,
        with_hierarchy
    from information_schema.role_table_grants
    where table_catalog = current_database()
) a;


/******************************************************************************
 * policy
 *****************************************************************************/

create view policy as
select policy_id(n.nspname, c.relname, p.polname) as id,
    p.polname::text as name,
    relation_id(n.nspname, c.relname) as relation_id,
    c.relname::text as relation_name,
    n.nspname::text as schema_name,
    p.polcmd::char::siuda as command,
    pg_get_expr(p.polqual, p.polrelid, True) as using,
    pg_get_expr(p.polwithcheck, p.polrelid, True) as check
from pg_policy p
    join pg_class c on c.oid = p.polrelid
    join pg_namespace n on n.oid = c.relnamespace;


/******************************************************************************
 * policy_role
 *****************************************************************************/
create view policy_role as
select
--    policy_id((relation_id).schema_name, (relation_id).name, policy_name)::text || '<-->' || role_id::text as id,
    policy_id((relation_id).schema_name, (relation_id).name, policy_name) as id,
    policy_name::text,
    relation_id,
    (relation_id).name as relation_name,
    (relation_id).schema_name as schema_name,
    role_id,
    (role_id).name as role_name
from (
    select
        p.polname as policy_name,
        relation_id(n.nspname, c.relname) as relation_id,
        unnest(p.polroles::regrole[]::text[]::role_id[]) as role_id
    from pg_policy p
        join pg_class c on c.oid = p.polrelid
        join pg_namespace n on n.oid = c.relnamespace
) a;


/******************************************************************************
 * constraint_unique
 *****************************************************************************/
create view constraint_unique as
    select constraint_id(tc.table_schema, tc.table_name, tc.constraint_name) as id,
           relation_id(tc.table_schema, tc.table_name) as table_id,
           tc.table_schema::text as schema_name,
           tc.table_name::text as table_name,
           tc.constraint_name::text as name,
           array_agg(column_id(ccu.table_schema, ccu.table_name, ccu.column_name)) as column_ids,
           array_agg(ccu.column_name::text) as column_names

    from information_schema.table_constraints tc

    inner join information_schema.constraint_column_usage ccu
            on ccu.constraint_catalog = tc.constraint_catalog and
               ccu.constraint_schema = tc.constraint_schema and
               ccu.constraint_name = tc.constraint_name

    where constraint_type = 'UNIQUE'

    group by tc.table_schema, tc.table_name, tc.constraint_name;


/******************************************************************************
 * constraint_check
 *****************************************************************************/
create view constraint_check as
    select constraint_id(tc.table_schema, tc.table_name, tc.constraint_name) as id,
           relation_id(tc.table_schema, tc.table_name) as table_id,
           tc.table_schema::text as schema_name,
           tc.table_name::text as table_name,
           tc.constraint_name::text as name,
           cc.check_clause::text

    from information_schema.table_constraints tc

    inner join information_schema.check_constraints cc
            on cc.constraint_catalog = tc.constraint_catalog and
               cc.constraint_schema = tc.constraint_schema and
               cc.constraint_name = tc.constraint_name;


/******************************************************************************
 * extension
 *****************************************************************************/
create view extension as
    select extension_id(ext.extname) as id,
           schema_id(pgn.nspname) as schema_id,
           pgn.nspname::text as schema_name,
           ext.extname::text as name,
           ext.extversion as version

    from pg_catalog.pg_extension ext
    inner join pg_catalog.pg_namespace pgn
            on pgn.oid = ext.extnamespace;


/******************************************************************************
 * foreign_data_wrapper
 *****************************************************************************/
create view foreign_data_wrapper as
    select id,
           name::text,
--            handler_id,
--            validator_id,
           opt as options

    from (
        select foreign_data_wrapper_id(fdwname) as id,
               fdwname as name,
--                h_f.id as handler_id,
--                v_f.id as validator_id,
               jsonb_build_object(variadic string_to_array(unnest(coalesce(fdwoptions, array['']::text[])), '=')) as opt

        from pg_catalog.pg_foreign_data_wrapper

        left join pg_proc p_h
               on p_h.oid = fdwhandler

        left join pg_namespace h_n
               on h_n.oid = p_h.pronamespace

--         left join function h_f
--                on h_f.schema_name = h_n.nspname and
--                   h_f.name = p_h.proname

        left join pg_proc p_v
               on p_v.oid = fdwvalidator

        left join pg_namespace v_n
               on v_n.oid = p_v.pronamespace

--         left join "function" v_f
--                on v_f.schema_name = v_n.nspname and
--                   v_f.name = p_v.proname
    ) q

    group by id,
             opt,
             name;
--              handler_id,
--              validator_id;


/******************************************************************************
 * foreign_server
 *****************************************************************************/
create view foreign_server as
    select id,
           foreign_data_wrapper_id,
           name::text,
           "type",
           version,
           opt as options

    from (
        select foreign_server_id(srvname) as id,
               foreign_data_wrapper_id(fdwname) as foreign_data_wrapper_id,
               srvname as name,
               srvtype as "type",
               srvversion as version,
               jsonb_build_object(variadic string_to_array(unnest(coalesce(srvoptions, array['']::text[])), '=')) as opt

        from pg_catalog.pg_foreign_server fs
        inner join pg_catalog.pg_foreign_data_wrapper fdw
                on fdw.oid = fs.srvfdw
    ) q

    group by id,
             opt,
             foreign_data_wrapper_id,
             name,
             "type",
             version;



/******************************************************************************
 * foreign_table
 *****************************************************************************/
create view foreign_table as
    select id,
           foreign_server_id,
           schema_id,
           schema_name::text,
           name::text,
           opt as options

    from (
        select relation_id(pgn.nspname, pgc.relname) as id,
               schema_id(pgn.nspname) as schema_id,
               foreign_server_id(pfs.srvname) as foreign_server_id,
               pgn.nspname as schema_name,
               pgc.relname as name,
               jsonb_build_object(variadic string_to_array(unnest(coalesce(ftoptions, array['']::text[])), '=')) as opt

        from pg_catalog.pg_foreign_table pft
        inner join pg_catalog.pg_class pgc
                on pgc.oid = pft.ftrelid
        inner join pg_catalog.pg_namespace pgn
                on pgn.oid = pgc.relnamespace
        inner join pg_catalog.pg_foreign_server pfs
                on pfs.oid = pft.ftserver
    ) q

    group by id,
             opt,
             schema_id,
             foreign_server_id,
             schema_name,
             name;


/******************************************************************************
 * foreign_column
 *****************************************************************************/

create view foreign_column as
    select column_id(c.table_schema, c.table_name, c.column_name) as id,
           relation_id(c.table_schema, c.table_name) as foreign_table_id,
           c.table_schema::text as schema_name,
           c.table_name::text as foreign_table_name,
           c.column_name::text as name,
           quote_ident(c.udt_schema) || '.' || quote_ident(c.udt_name) as "type",
           (c.is_nullable = 'YES') as nullable

    from pg_catalog.pg_foreign_table pft
    inner join pg_catalog.pg_class pgc
            on pgc.oid = pft.ftrelid
    inner join pg_catalog.pg_namespace pgn
            on pgn.oid = pgc.relnamespace
    inner join information_schema.columns c
            on c.table_schema = pgn.nspname and
               c.table_name = pgc.relname;

/******************************************************************************
 * index
 *****************************************************************************/
create view "index" as
select index_id(ns.nspname, c.relname) as id,
       ns.nspname as schema_name,
       c.relname as name
from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace;

create view "index_relation" as
select index_id(ns.nspname, c.relname) as id,
       relation_id(rns.nspname, rc.relname)
from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
         inner join pg_class rc on rc.oid = i.indrelid
         inner join pg_namespace rns on rns.oid = rc.relnamespace;

create view "index_unique" as
select index_id(ns.nspname, c.relname) as id
from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
where i.indisunique;

if current_setting('server_version_num')::int / 10000 > 14 then
create view "index_unique_null_values_distinct" as
select index_id(ns.nspname, c.relname) as id
from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
where i.indisunique
  and not i.indnullsnotdistinct;
else
create view "index_unique_null_values_distinct" as
    select index_id(ns.nspname, c.relname) as id
    from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace;
end if;

create view "index_primary_key" as
select index_id(ns.nspname, c.relname) as id
from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
where i.indisprimary;

create view "index_unique_immediate" as
select index_id(ns.nspname, c.relname) as id
from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
where i.indisunique
  and i.indimmediate;

create view "index_replica_identity" as
    select index_id(ns.nspname, c.relname) as id
    from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
    where i.indisreplident;

create view "index_attribute" as
    select index_id(ns.nspname, c.relname) as id,
           attribute, position
    from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
    inner join lateral (select position, pg_get_indexdef(indexrelid::oid, position, true) as attribute from generate_series(1,indnatts) position) t
    on true
where indnatts > 0;

create view "index_partial" as
    select index_id(ns.nspname, c.relname) as id,
           pg_get_expr(indpred, indrelid) as condition
    from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
    where i.indpred is not null;
