/*
This code is partially based on meta from the Aquameta project (https://github.com/aquameta/meta),
licensed under the terms of BSD-2 Clause License. It started off as an integration but became a
significant re-working.
*/

-- returns an array of types, given an identity_args string (as provided by pg_get_function_identity_arguments())
create or replace function _get_function_type_sig_array(p pg_proc) returns text[]
    language sql as
$$
select
    coalesce(array_agg(format_type(typ, null) order by ordinality), '{}')
from
    (select
         typ,
         ordinality
     from
         unnest(p.proargtypes) with ordinality as t(typ, ordinality)) as arg(typ, ordinality)
$$ set search_path = '', stable;

create function resolved_type_name(t pg_type) returns name
stable
return case
           when t.typelem != 0 and t.typlen = -1
               then (select
                         typname || '[]'
                     from
                         pg_type bt
                     where
                         bt.oid = t.typelem)
           else t.typname
    end;

create function _pg_get_expr(pg_node_tree, oid) returns text
 set search_path = ''
stable
 return pg_get_expr($1, $2);

create function _pg_get_expr(pg_node_tree, oid, boolean) returns text
 set search_path = ''
stable
 return pg_get_expr($1, $2, $3);


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
    select
        type_id(n.nspname, resolved_type_name(t)) as id,
        n.nspname::text                           as schema_name,
        case
            when t.typelem != 0 and t.typlen = -1
                then (select typname || '[]' from pg_type bt where bt.oid = t.typelem)
            else t.typname
            end                                   as name
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace;


create or replace view type_basic as
    select
        type_id(n.nspname, resolved_type_name(t)) as id
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
    where
        typtype = 'b';

create or replace view type_array as
    select
        type_id(n.nspname, resolved_type_name(t))   as id,
        type_id(n1.nspname, resolved_type_name(t1)) as base_type_id
    from
        pg_catalog.pg_type      t
        inner join pg_type      t1 on t1.oid = t.typelem
        inner join pg_namespace n on n.oid = t.typnamespace
        inner join pg_namespace n1 on n1.oid = t1.typnamespace
    where
        t.typelem != 0;

create or replace view type_composite as
    select
        type_id(n.nspname, resolved_type_name(t)) as id
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
    where
        typtype = 'c';

create or replace view type_composite_attribute as
    select
        type_id(n.nspname, resolved_type_name(t)) as id,
        a.attname                                 as name
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
        join      pg_attribute            a on a.attrelid = t.typrelid
    where
        t.typtype = 'c' and
        a.attnum > 0 and
        not a.attisdropped;

create or replace view type_composite_attribute_position as
    select
        type_id(n.nspname, resolved_type_name(t)) as id,
        a.attname                                 as name,
        a.attnum                                  as position
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
        join      pg_attribute            a on a.attrelid = t.typrelid
    where
        t.typtype = 'c' and
        a.attnum > 0 and
        not a.attisdropped;

create or replace view type_composite_attribute_collation as
    select
        type_id(n.nspname, resolved_type_name(t)) as id,
        a.attname                                 as name,
        co.collname                               as collation
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
        join      pg_attribute            a on a.attrelid = t.typrelid
        join      pg_type                 ct on ct.oid = a.atttypid
        left join pg_collation            co on co.oid = a.attcollation
    where
        t.typtype = 'c' and
        a.attnum > 0 and
        not a.attisdropped and
        a.attcollation <> ct.typcollation;

create or replace view type_domain as
    select
        type_id(n.nspname, resolved_type_name(t))   as id,
        type_id(n1.nspname, resolved_type_name(t1)) as base_type_id
    from
        pg_catalog.pg_type                 t
        left join  pg_catalog.pg_namespace n on n.oid = t.typnamespace
        inner join pg_catalog.pg_type      t1 on t1.oid = t.typbasetype
        left join  pg_catalog.pg_namespace n1 on n1.oid = t1.typnamespace
    where
        t.typtype = 'd';

create or replace view type_enum as
    select
        type_id(n.nspname, resolved_type_name(t)) as id
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
    where
        typtype = 'e';

create or replace view type_enum_label as
    select
        type_id(n.nspname, resolved_type_name(t)) as id,
        enumlabel                                 as label,
        enumsortorder                             as sortorder
    from
        pg_catalog.pg_type                 t
        left join  pg_catalog.pg_namespace n on n.oid = t.typnamespace
        inner join pg_enum                 e on e.enumtypid = t.oid
    where
        typtype = 'e';

create or replace view type_pseudo as
    select
        type_id(n.nspname, resolved_type_name(t)) as id
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
    where
        typtype = 'p';

create or replace view type_range as
    select
        type_id(n.nspname, resolved_type_name(t)) as id
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
    where
        typtype = 'r';

create or replace view type_multirange as
    select
        type_id(n.nspname, resolved_type_name(t)) as id
    from
        pg_catalog.pg_type                t
        left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
    where
        typtype = 'm';



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
            _get_function_type_sig_array(p)
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
           type_id(tns.nspname, resolved_type_name(t))
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
    select
        relation_id(ns.nspname, c.relname) as id,
        schema_id(ns.nspname)              as schema_id,
        ns.nspname                         as schema_name,
        c.relname                          as name
    from
        pg_class                c
        inner join pg_namespace ns on ns.oid = c.relnamespace
    where
        c.relkind = 'r' or
        c.relkind = 'p';

create view table_partitioned as
    select
        relation_id(ns.nspname, c.relname) as id
    from
        pg_class                c
        inner join pg_namespace ns on ns.oid = c.relnamespace
    where
        c.relkind = 'p';

create view table_permanent as
    select
        relation_id(ns.nspname, c.relname) as id
    from
        pg_class                c
        inner join pg_namespace ns on ns.oid = c.relnamespace
    where
        c.relkind = 'r' and
        c.relpersistence = 'p';

create view table_temporary as
    select
        relation_id(ns.nspname, c.relname) as id
    from
        pg_class                c
        inner join pg_namespace ns on ns.oid = c.relnamespace
    where
        c.relkind = 'r' and
        c.relpersistence = 't';

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
    select
        column_id(ns.nspname, c.relname, a.attname)                                                     as id,
        cast(case when a.attgenerated = '' then _pg_get_expr(ad.adbin, ad.adrelid) end as text) as "default"
    from
        pg_attribute a inner join pg_attrdef ad on attrelid = adrelid and attnum = adnum
        inner join pg_class                                                               c on a.attrelid = c.oid
            and c.relkind = any ('{r,v,m,f,p}')
        inner join pg_namespace                                                           ns on ns.oid = c.relnamespace
    where
        a.attnum > 0;

create view relation_column_identity as
    select
        column_id(ns.nspname, c.relname, a.attname)                                                     as id,
        a.attidentity = 'a' as always
    from
        pg_attribute a
        inner join pg_class                                                               c on a.attrelid = c.oid
            and c.relkind = any ('{r,v,m,f,p}')
        inner join pg_namespace                                                           ns on ns.oid = c.relnamespace
    where
        a.attnum > 0 and a.attidentity in ('a','d');

create view relation_column_generated as
    select
        column_id(ns.nspname, c.relname, a.attname)                                                     as id,
        _pg_get_expr(ad.adbin, ad.adrelid)::text as generated
    from
        pg_attribute a inner join pg_attrdef ad on attrelid = adrelid and attnum = adnum
        inner join pg_class                                                               c on a.attrelid = c.oid
            and c.relkind = any ('{r,v,m,f,p}')
        inner join pg_namespace                                                           ns on ns.oid = c.relnamespace
    where
        a.attnum > 0 and a.attgenerated in ('s');


create view relation_column_type as
    select
        column_id(ns.nspname, c.relname, col.attname)                                                      as id,
        type_id(nt.nspname, resolved_type_name(t)) as "type_id"
    from
        (pg_attribute a left join pg_attrdef ad on attrelid = adrelid and attnum = adnum) col
        inner join pg_class                                                               c on col.attrelid = c.oid
            and c.relkind = any('{r,v,m,f,p}')
        inner join pg_namespace                                                           ns on ns.oid = c.relnamespace
        join       (pg_type t join pg_namespace nt on (t.typnamespace = nt.oid)) on col.atttypid = t.oid
    where col.attnum > 0;

create view relation_column_nullable as
    select column_id(c.table_schema, c.table_name, c.column_name) as id
    from information_schema.columns c
        where c.is_nullable = 'YES';

/******************************************************************************
 * relation
 *****************************************************************************/
create view relation as
    select relation_id(ns.nspname, c.relname) as id,
           schema_id(ns.nspname) as schema_id,
           ns.nspname::text as schema_name,
           c.relname::text as name
    from pg_class c
    inner join pg_namespace ns on ns.oid = c.relnamespace
    where reltype != 0;

create view relation_primary_key as
    select relation_id(ns.nspname, c.relname) as id,
           column_id(ns.nspname, c.relname, a.attname),
           position
    from pg_class c
    inner join pg_namespace ns on ns.oid = c.relnamespace
    inner join pg_constraint ct on ct.conrelid = c.oid and ct.contype = 'p'
    join lateral (select * from unnest(ct.conkey) with ordinality as t(key, position)) as k on true
    inner join pg_attribute a on a.attrelid = c.oid and a.attnum = key
    where reltype != 0;

create view relation_foreign_key as
    select relation_id(ns.nspname, c.relname) as id,
           column_id(ns.nspname, c.relname, a.attname),
           position
    from pg_class c
    inner join pg_namespace ns on ns.oid = c.relnamespace
    inner join pg_constraint ct on ct.conrelid = c.oid and ct.contype = 'f'
    join lateral (select * from unnest(ct.conkey) with ordinality as t(key, position)) as k on true
    inner join pg_attribute a on a.attrelid = c.oid and a.attnum = key
    where reltype != 0;

create view relation_foreign_key_constraint as
    select relation_id(ns.nspname, c.relname) as id,
           constraint_id(ns.nspname, c.relname, ct.conname)
    from pg_class c
    inner join pg_namespace ns on ns.oid = c.relnamespace
    inner join pg_constraint ct on ct.conrelid = c.oid and ct.contype = 'f'
    join lateral (select * from unnest(ct.conkey) with ordinality as t(key, position)) as k on true
    inner join pg_attribute a on a.attrelid = c.oid and a.attnum = key
    where reltype != 0;

/******************************************************************************
 * function
 *****************************************************************************/

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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig,
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
                _get_function_type_sig_array(p) as type_sig,
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
        type_id(tns.nspname, resolved_type_name(t)) as type_id,
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
                _get_function_type_sig_array(p) as type_sig,
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
                _get_function_type_sig_array(p) as type_sig,
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
            _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig
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
                _get_function_type_sig_array(p) as type_sig,
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
                _get_function_type_sig_array(p) as type_sig,
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
                _get_function_type_sig_array(p) as type_sig,
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
                _get_function_type_sig_array(p) as type_sig,
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
                _get_function_type_sig_array(p) as type_sig,
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
        type_id(ns.nspname, resolved_type_name(t)) as return_type_id
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
           pg_trigger.tgname::text as name

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

    where
           not tgisinternal;

create view trigger_callable as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id,
           function_id(
               f_pgn.nspname,
               pgp.proname,
               _get_function_type_sig_array(pgp)
           ) as callable_id

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
    where
           not tgisinternal;

create view trigger_when as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id,
           case when (tgtype >> 1 & 1)::bool then 'before'
                when (tgtype >> 6 & 1)::bool then 'instead'
                else 'after'
           end as "when"

    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where 
           not tgisinternal;

create view trigger_insert as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal
           and (tgtype >> 2 & 1)::bool;

create view trigger_delete as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal
           and (tgtype >> 3 & 1)::bool;

create view trigger_update as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where

           not tgisinternal
           and (tgtype >> 4 & 1)::bool;

create view trigger_truncate as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal
           and (tgtype >> 5 & 1)::bool;

create view trigger_level as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id,
    case when (tgtype & 1)::bool then 'row'
         else 'statement'
    end as level

    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal;

create view trigger_enabled as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal
           and tgenabled <> 'D';

create view trigger_deferrable as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal
           and tgdeferrable;

create view trigger_initially_deferred as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal
           and tginitdeferred;

create view trigger_arguments as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id,
           tgnargs as number_of_arguments,
           (select array_agg(convert_from(decode(unnest, 'hex'), 'utf8')) from unnest(regexp_split_to_array(encode(tgargs, 'hex'), '00')) where unnest <> '') as arguments
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    where
           not tgisinternal;

create view trigger_columns as
    select trigger_id(t_pgn.nspname, pgc.relname, pg_trigger.tgname) as id,
           column_id(t_pgn.nspname, pgc.relname, pga.attname) as column_id
    from pg_trigger

    inner join pg_class pgc
            on pgc.oid = tgrelid

    inner join pg_namespace t_pgn
            on t_pgn.oid = pgc.relnamespace
    cross join lateral unnest(tgattr) attribute
    join pg_attribute pga
            on pga.attrelid = tgrelid
               and pga.attnum = attribute
    where
           not tgisinternal;

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
   from pg_roles pgr where pgr.rolvaliduntil is not null;

create view role_setting as
    select role_setting_id(pgr.rolname, pgd.datname, split_part(setting, '=', 1)) as id,
           role_id(pgr.rolname) as role_id,
           split_part(setting, '=', 1) as setting_name,
           split_part(setting, '=', 2) as setting_value
    from pg_db_role_setting pgrs
        join pg_roles pgr on pgr.oid = pgrs.setrole
        left join pg_database pgd on pgd.oid = pgrs.setdatabase,
        unnest(pgrs.setconfig) as setting;

create view role_member as
select
    role_id(r.rolname::text) as id,
    role_id(r2.rolname::text) as member_role_id
from pg_auth_members m
    join pg_roles r on r.oid = m.roleid
    join pg_roles r2 on r2.oid = m.member;


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
    _pg_get_expr(p.polqual, p.polrelid, True) as using,
    _pg_get_expr(p.polwithcheck, p.polrelid, True) as check
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
 * constraint_relation_non_null*
 *****************************************************************************/

create view constraint_relation_non_null_column as
    select
        constraint_id(n.nspname, r.relname, a.attname || '_not_null') as id,
        column_id(n.nspname, r.relname, a.attname)
    from
        pg_namespace n,
        pg_class     r,
        pg_attribute a
    where
        n.oid = r.relnamespace and
        r.oid = a.attrelid and
        a.attnum > 0 and
        not a.attisdropped and
        a.attnotnull and
        r.relkind in ('r', 'p') and
        pg_has_role(r.relowner, 'USAGE');

create view constraint_relation_non_null as
    select
        constraint_id(n.nspname, r.relname, a.attname || '_not_null') as id,
        relation_id(n.nspname, r.relname),
        a.attname || '_not_null'                                      as name
    from
        pg_namespace n,
        pg_class     r,
        pg_attribute a
    where
        n.oid = r.relnamespace and
        r.oid = a.attrelid and
        a.attnum > 0 and
        not a.attisdropped and
        a.attnotnull and
        r.relkind in ('r', 'p') and
        pg_has_role(r.relowner, 'USAGE');


/******************************************************************************
 * constraint_relation_check*
 *****************************************************************************/
create view constraint_relation_check as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        relation_id(cns.nspname, cc.relname),
        c.conname                                        as name

    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'c'
    union all
    select *
    from
        constraint_relation_non_null;

create view constraint_relation_check_expr as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        _pg_get_expr(c.conbin, coalesce(cc.oid, 0))       as expr

    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'c'
    union all
    select
        id,
        (column_id).name || ' IS NOT NULL' as expr
    from
        constraint_relation_non_null_column cruc;

/******************************************************************************
 * constraint_relation_unique*
 *****************************************************************************/
create view constraint_relation_unique as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        relation_id(cns.nspname, cc.relname),
        c.conname                                        as name

    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'u';

create view constraint_relation_unique_column as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        column_id(ns.nspname, cc.relname, a.attname)
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
        inner join pg_attribute a on a.attrelid = cc.oid and a.attnum = any(c.conkey)
    where
        c.contype = 'u';

/******************************************************************************
 * constraint_relation_foreign_key*
 *****************************************************************************/
create view constraint_relation_foreign_key as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        relation_id(cns.nspname, cc.relname),
        c.conname                                        as name

    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'f';

create view constraint_relation_foreign_key_update as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        (case when c.confupdtype = 'a' then 'no action'
         when c.confupdtype = 'r' then 'restrict'
         when c.confupdtype = 'c' then 'cascade'
         when c.confupdtype = 'n' then 'set null'
         when c.confupdtype = 'd' then 'set default'
         else null end) as update_action
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'f';

create view constraint_relation_foreign_key_delete as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        (case when c.confdeltype = 'a' then 'no action'
         when c.confdeltype = 'r' then 'restrict'
         when c.confdeltype = 'c' then 'cascade'
         when c.confdeltype = 'n' then 'set null'
         when c.confdeltype = 'd' then 'set default'
         else null end) as delete_action
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'f';

create view constraint_relation_foreign_key_match_full as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'f' and c.confmatchtype = 'f';

create view constraint_relation_foreign_key_match_partial as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'f' and c.confmatchtype = 'p';

create view constraint_relation_foreign_key_match_simple as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
    where
        c.contype = 'f' and c.confmatchtype = 's';

create view constraint_relation_foreign_key_references as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        relation_id(rns.nspname, rc.relname)
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
        inner join pg_class rc on rc.oid = c.confrelid
        inner join pg_namespace rns on rns.oid = rc.relnamespace
    where
        c.contype = 'f';

create view constraint_relation_foreign_key_references_column as
    select
        constraint_id(ns.nspname, cc.relname, c.conname) as id,
        column_id(rns.nspname, rc.relname, a.attname),
        position
    from
        pg_constraint           c
        inner join pg_namespace ns on ns.oid = c.connamespace
        inner join pg_class     cc on cc.oid = c.conrelid
        inner join pg_namespace cns on cns.oid = cc.relnamespace
        inner join pg_class rc on rc.oid = c.confrelid
        inner join pg_namespace rns on rns.oid = rc.relnamespace
        join lateral (select * from unnest(c.confkey) with ordinality as t(key, position)) as k on true
        inner join pg_attribute a on a.attrelid = rc.oid and a.attnum = key
    where
        c.contype = 'f';
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
           _pg_get_expr(indpred, indrelid) as condition
    from pg_index i
         inner join pg_class c on c.oid = i.indexrelid
         inner join pg_class cr on cr.oid = i.indrelid and cr.relkind != 't'
         inner join pg_namespace ns on ns.oid = c.relnamespace
    where i.indpred is not null;

--- language

create view language as
    select language_id(lanname) as id
from pg_language;

create view language_trusted as
    select language_id(lanname) as id
from pg_language where lanpltrusted;

create view language_internal as
    select language_id(lanname) as id
from pg_language where not lanispl;

create view language_handler as
    select language_id(lanname) as id,
           function_id(ns.nspname, p.proname, _get_function_type_sig_array(p)) as handler_id
from pg_language l
     inner join pg_proc p on p.oid = l.lanplcallfoid
     inner join pg_namespace ns on ns.oid = p.pronamespace
where lanispl;

create view language_inline_handler as
    select language_id(lanname) as id,
           function_id(ns.nspname, p.proname, _get_function_type_sig_array(p)) as handler_id
from pg_language l
     inner join pg_proc p on p.oid = l.laninline
     inner join pg_namespace ns on ns.oid = p.pronamespace
where lanispl and laninline != 0;

create view language_validator as
    select language_id(lanname) as id,
           function_id(ns.nspname, p.proname, _get_function_type_sig_array(p)) as handler_id
from pg_language l
     inner join pg_proc p on p.oid = l.laninline
     inner join pg_namespace ns on ns.oid = p.pronamespace
where lanispl and lanvalidator != 0;

create view language_acl as
    select language_id(lanname) as id,
           aclitem as acl
from pg_language
  inner join lateral (select * from unnest(lanacl)) acl(aclitem) on true;

-- TODO: language owner

--- descriptions/comments

create view comment as
    -- relation
    select
        relation_id(ns.nspname, c.relname)::object_id as id,
        description                                   as comment
    from
        pg_description          d
        inner join pg_class     c on c.oid = d.objoid and d.classoid = 'pg_class'::regclass
        inner join pg_namespace ns on ns.oid = c.relnamespace
        where d.objsubid = 0
    union all
    -- callable
    select
        function_id(ns.nspname, p.proname, _get_function_type_sig_array(p))::object_id as id,
        description                                   as comment
    from
        pg_description          d
        inner join pg_proc     p on p.oid = d.objoid and d.classoid = 'pg_proc'::regclass
        inner join pg_namespace ns on ns.oid = p.pronamespace
    union all
    -- column
    select
        column_id(ns.nspname, c.relname, a.attname)::object_id as id,
        description                                   as comment
    from
        pg_description          d
        inner join pg_class     c on c.oid = d.objoid and d.classoid = 'pg_class'::regclass
        inner join pg_attribute a on a.attrelid = c.oid and a.attnum > 0
        inner join pg_namespace ns on ns.oid = c.relnamespace
        where d.objsubid != 0
    union all
    -- cast
    select
        cast_id(st_ns.nspname, st.typname, tt_ns.nspname, tt.typname)::object_id as id,
        description                                                              as comment
    from
        pg_description          d
        inner join pg_cast      c on c.oid = d.objoid and d.classoid = 'pg_cast'::regclass
        inner join pg_type      st on st.oid = c.castsource
        inner join pg_type      tt on tt.oid = c.casttarget
        inner join pg_namespace st_ns on st_ns.oid = st.typnamespace
        inner join pg_namespace tt_ns on tt_ns.oid = tt.typnamespace;

--- dependencies

create view obj_object_id as
    select
        'pg_class'::regclass::oid                     as classid,
        c.oid                                         as objid,
        0::oid                                        as objsubid,
        relation_id(ns.nspname, c.relname)::object_id as object_id
    from
        pg_class                c
        inner join pg_namespace ns on ns.oid = c.relnamespace
    union all
    select
        'pg_extension'::regclass::oid      as classid,
        e.oid                              as objid,
        0::oid                             as objsubid,
        extension_id(e.extname)::object_id as object_id
    from
        pg_extension            e
        inner join pg_namespace ns on ns.oid = e.extnamespace
    union all
    select

        'pg_type'::regclass::oid                              as classid,
        t.oid                                                 as objid,
        0::oid                                                as objsubid,
        type_id(ns.nspname, resolved_type_name(t))::object_id as object_id
    from
        pg_type                 t
        inner join pg_namespace ns on ns.oid = t.typnamespace
    union all
    select
        'pg_namespace'::regclass::oid    as classid,
        ns.oid                           as objid,
        0::oid                           as objsubid,
        schema_id(ns.nspname)::object_id as object_id
    from
        pg_namespace ns
    union all
    select
        'pg_proc'::regclass::oid                                                       as classid,
        p.oid                                                                          as objid,
        0::oid                                                                         as objsubid,
        function_id(ns.nspname, p.proname, _get_function_type_sig_array(p))::object_id as object_id
    from
        pg_proc                 p
        inner join pg_namespace ns on ns.oid = p.pronamespace
    union all
    select
        'pg_language'::regclass::oid    as classid,
        l.oid                           as objid,
        0::oid                          as objsubid,
        language_id(lanname)::object_id as object_id
    from
        pg_language l;
--- TODO: handle the rest of the cases (^^)

CREATE TABLE mv_refresh_log (
	view_name text NOT NULL,
	refresh_time timestamp DEFAULT CURRENT_TIMESTAMP NULL,
	last_max_xid int8 NULL,
	CONSTRAINT mv_refresh_log_pkey PRIMARY KEY (view_name)
);

CREATE TYPE pg_depend_type AS (
	classid oid,
	objid oid,
	objsubid int4,
	refclassid oid,
	refobjid oid,
	refobjsubid int4,
	deptype char);


CREATE MATERIALIZED VIEW dependency_pre_mv
AS WITH depend_data AS (
         SELECT pg_depend.classid,
            pg_depend.objid,
            pg_depend.objsubid,
            pg_depend.refclassid,
            pg_depend.refobjid,
            pg_depend.refobjsubid,
            pg_depend.deptype
           FROM pg_depend
          WHERE pg_depend.deptype <> 'i'::"char"
        )
 SELECT DISTINCT id,
    classid,
    objid,
    objsubid,
    refclassid,
    refobjid,
    refobjsubid,
    deptype
   FROM ( SELECT DISTINCT relation_id(ns.nspname, c.relname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_class c ON c.oid = d.objid AND d.classid = 'pg_class'::regclass::oid AND c.relkind <> 't'::"char"
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT function_id(ns.nspname, p.proname, _get_function_type_sig_array(p.*)::name[])::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_proc p ON p.oid = d.objid AND d.classid = 'pg_proc'::regclass::oid
             JOIN pg_namespace ns ON ns.oid = p.pronamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT column_id(ns.nspname, c.relname, a.attname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_class c ON c.oid = d.objid AND d.classid = 'pg_class'::regclass::oid
             JOIN pg_attribute a ON a.attrelid = c.oid AND a.attnum > 0
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
          WHERE d.objsubid <> 0
        UNION ALL
         SELECT DISTINCT column_id(ns.nspname, c.relname, a.attname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_class c ON c.oid = d.refobjid AND d.refclassid = 'pg_class'::regclass::oid
             JOIN pg_attribute a ON a.attrelid = c.oid AND a.attnum > 0
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
          WHERE d.objsubid <> 0
        UNION ALL
         SELECT DISTINCT cast_id(st_ns.nspname, st.typname, tt_ns.nspname, tt.typname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_cast c ON c.oid = d.objid AND d.classid = 'pg_cast'::regclass::oid
             JOIN pg_type st ON st.oid = c.castsource
             JOIN pg_type tt ON tt.oid = c.casttarget
             JOIN pg_namespace st_ns ON st_ns.oid = st.typnamespace
             JOIN pg_namespace tt_ns ON tt_ns.oid = tt.typnamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT type_id(ns.nspname, t.typname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_type t ON t.oid = d.objid AND d.classid = 'pg_type'::regclass::oid
             JOIN pg_namespace ns ON ns.oid = t.typnamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT type_id(ns.nspname, t.typname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_type t ON t.oid = d.objid AND d.classid = 'pg_type'::regclass::oid AND t.typcategory = 'A'::"char"
             JOIN pg_namespace ns ON ns.oid = t.typnamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT relation_id(ns.nspname, c.relname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_class c ON c.oid = d.objid AND d.classid = 'pg_class'::regclass::oid
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT relation_id(ns.nspname, c.relname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_class c ON c.oid = d.objid AND d.classid = 'pg_class'::regclass::oid AND c.relkind = 'r'::"char"
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT operator_id(ns.nspname, o.oprname, lns.nspname, resolved_type_name(lt.*), rns.nspname, resolved_type_name(rt.*))::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_operator o ON o.oid = d.objid AND d.classid = 'pg_operator'::regclass::oid
             JOIN pg_namespace ns ON ns.oid = o.oprnamespace
             LEFT JOIN pg_type lt ON lt.oid = o.oprleft
             LEFT JOIN pg_namespace lns ON lns.oid = lt.typnamespace
             LEFT JOIN pg_type rt ON rt.oid = o.oprright
             LEFT JOIN pg_namespace rns ON rns.oid = rt.typnamespace
          WHERE d.objsubid = 0
        UNION ALL
         SELECT DISTINCT sequence_id(ns.nspname, c.relname)::object_id AS id,
            d.classid,
            d.objid,
            d.objsubid,
            d.refclassid,
            d.refobjid,
            d.refobjsubid,
            d.deptype
           FROM depend_data d
             JOIN pg_class c ON c.oid = d.objid AND d.classid = 'pg_class'::regclass::oid
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
          WHERE d.objsubid = 0) union_results
WITH DATA;

CREATE UNIQUE INDEX idx_dependency_pre_mv_unique ON dependency_pre_mv USING btree (id, classid, objid, objsubid, refclassid, refobjid, refobjsubid, deptype);

CREATE OR REPLACE FUNCTION check_and_refresh_mv(mv_name TEXT)
RETURNS void
LANGUAGE plpgsql
AS $function$
DECLARE
    current_max_xid BIGINT;
    last_max_xid BIGINT;
    current_max_xmin BIGINT;
    is_refresh_in_progress BOOLEAN;
    dblink_connection TEXT;
BEGIN
    -- Ensure we have fresh visibility of catalog changes
    PERFORM pg_stat_clear_snapshot();

    -- Fetch the last stored max XID from the log table for the specified mv_name
    SELECT COALESCE(mv_refresh_log.last_max_xid, 0)
    INTO last_max_xid
    FROM mv_refresh_log
    WHERE view_name = mv_name
    FOR UPDATE;  -- Prevent race conditions

    -- If no record exists, initialize and insert a default entry
    IF last_max_xid = 0 THEN
        INSERT INTO mv_refresh_log (view_name, last_max_xid)
        VALUES (mv_name, last_max_xid)
        ON CONFLICT(view_name) DO UPDATE
        SET last_max_xid = EXCLUDED.last_max_xid;
    END IF;

    -- Determine how to track changes based on mv_name
    IF mv_name = 'acl_mv' THEN
        -- Use relminmxid from pg_class for acl_mv
        SELECT COALESCE(MAX(c.relminmxid::text::BIGINT), 0)  
        INTO current_max_xmin
        FROM pg_class c
        JOIN pg_namespace n ON c.relnamespace = n.oid
        WHERE c.relname = mv_name AND n.nspname = 'omni_schema';
    ELSIF mv_name = 'dependency_pre_mv' THEN
        -- Use xmin from pg_depend for dependency_pre_mv
        SELECT COALESCE(MAX(xmin::text::BIGINT), 0)
        INTO current_max_xmin
        FROM pg_depend;
    ELSE
        RAISE EXCEPTION 'Unknown materialized view name: %', mv_name;
    END IF;

    -- Debugging output
    RAISE NOTICE 'Checking materialized view: %, last_max_xid: %, current_max_xmin: %', mv_name, last_max_xid, current_max_xmin;

    -- Check if another refresh operation is already in progress
    SELECT EXISTS (
        SELECT 1
        FROM pg_stat_activity
        WHERE state = 'active' 
          AND query LIKE 'REFRESH MATERIALIZED VIEW%'
          AND query LIKE '%' || mv_name || '%'
    ) INTO is_refresh_in_progress;

    IF is_refresh_in_progress THEN
        RAISE NOTICE 'Refresh already in progress for materialized view %, skipping.', mv_name;
        RETURN;  -- Skip refresh
    END IF;

    BEGIN
        -- Only refresh if the new max_xmin is greater than the last recorded XID
        IF current_max_xmin > last_max_xid THEN
            RAISE NOTICE 'Materialized view % is stale. Refreshing...', mv_name;

            -- Perform the refresh
            EXECUTE format('SELECT dblink_exec($$REFRESH MATERIALIZED VIEW omni_schema.%I$$)', mv_name);

            -- Update refresh log
            UPDATE mv_refresh_log
            SET last_max_xid = current_max_xmin
            WHERE view_name = mv_name;

            RAISE NOTICE 'Materialized view % refreshed successfully.', mv_name;
        ELSE
            RAISE NOTICE 'Materialized view % is up to date. No refresh needed.', mv_name;
        END IF;
    EXCEPTION
        WHEN OTHERS THEN
            RAISE WARNING 'Error during refresh: %', SQLERRM;
            RAISE;
    END;

END;
$function$;


CREATE OR REPLACE FUNCTION check_and_refresh_mv_once(mv_name text)
 RETURNS boolean
 LANGUAGE plpgsql
AS $function$
BEGIN
    -- Acquire a lock to prevent multiple executions
    PERFORM pg_advisory_lock(hashtext(mv_name));

    -- Run the refresh only if the lock is acquired
    PERFORM check_and_refresh_mv(mv_name);

    -- Release the lock
    PERFORM pg_advisory_unlock(hashtext(mv_name));

    RETURN true;
END;
$function$
;

CREATE OR REPLACE VIEW dependency
AS WITH refresh AS (
         SELECT check_and_refresh_mv_once('dependency_pre_mv') AS refreshed
        )
 SELECT sub.id,
    oo.object_id AS dependent_on
   FROM dependency_pre_mv sub
     JOIN obj_object_id oo ON oo.classid = sub.refclassid AND oo.objid = sub.refobjid AND oo.objsubid = sub.refobjsubid::oid
     JOIN refresh ON true;


CREATE MATERIALIZED VIEW acl_mv
AS WITH dedup AS (
         SELECT function_id(ns.nspname, p.proname, _get_function_type_sig_array(p.*)::name[])::object_id AS id,
            acl.grantor,
            acl.grantee,
            acl.privilege_type,
            acl.is_grantable,
            acl."default",
            row_number() OVER (PARTITION BY (function_id(ns.nspname, p.proname, _get_function_type_sig_array(p.*)::name[])::object_id), acl.grantor, acl.grantee, acl.privilege_type ORDER BY p.proowner) AS rn
           FROM pg_proc p
             JOIN pg_namespace ns ON ns.oid = p.pronamespace
             JOIN LATERAL ( SELECT role_id(aclexplode.grantor::regrole::name) AS grantor,
                    role_id(aclexplode.grantee::regrole::name) AS grantee,
                    aclexplode.privilege_type,
                    aclexplode.is_grantable,
                    p.proacl IS NULL AS "default"
                   FROM aclexplode(COALESCE(p.proacl, acldefault('f'::"char", p.proowner))) aclexplode(grantor, grantee, privilege_type, is_grantable)) acl ON true
        UNION ALL
         SELECT type_id(ns.nspname, resolved_type_name(t.*))::object_id AS id,
            acl.grantor,
            acl.grantee,
            acl.privilege_type,
            acl.is_grantable,
            acl."default",
            row_number() OVER (PARTITION BY (type_id(ns.nspname, resolved_type_name(t.*))::object_id), acl.grantor, acl.grantee, acl.privilege_type ORDER BY t.typowner) AS rn
           FROM pg_type t
             JOIN pg_namespace ns ON ns.oid = t.typnamespace
             JOIN LATERAL ( SELECT role_id(aclexplode.grantor::regrole::name) AS grantor,
                    role_id(aclexplode.grantee::regrole::name) AS grantee,
                    aclexplode.privilege_type,
                    aclexplode.is_grantable,
                    t.typacl IS NULL AS "default"
                   FROM aclexplode(COALESCE(t.typacl, acldefault('T'::"char", t.typowner))) aclexplode(grantor, grantee, privilege_type, is_grantable)) acl ON true
        UNION ALL
         SELECT column_id(ns.nspname, c.relname, a.attname)::object_id AS id,
            acl.grantor,
            acl.grantee,
            acl.privilege_type,
            acl.is_grantable,
            acl."default",
            row_number() OVER (PARTITION BY (column_id(ns.nspname, c.relname, a.attname)::object_id), acl.grantor, acl.grantee, acl.privilege_type ORDER BY a.attrelid) AS rn
           FROM pg_attribute a
             JOIN pg_class c ON c.oid = a.attrelid AND c.reltype <> 0::oid
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
             JOIN LATERAL ( SELECT role_id(aclexplode.grantor::regrole::name) AS grantor,
                    role_id(aclexplode.grantee::regrole::name) AS grantee,
                    aclexplode.privilege_type,
                    aclexplode.is_grantable,
                    a.attacl IS NULL AS "default"
                   FROM aclexplode(COALESCE(a.attacl, acldefault('c'::"char", c.relowner))) aclexplode(grantor, grantee, privilege_type, is_grantable)) acl ON true
        UNION ALL
         SELECT relation_id(ns.nspname, c.relname)::object_id AS id,
            acl.grantor,
            acl.grantee,
            acl.privilege_type,
            acl.is_grantable,
            acl."default",
            row_number() OVER (PARTITION BY (relation_id(ns.nspname, c.relname)::object_id), acl.grantor, acl.grantee, acl.privilege_type ORDER BY c.relowner) AS rn
           FROM pg_class c
             JOIN pg_namespace ns ON ns.oid = c.relnamespace
             JOIN LATERAL ( SELECT role_id(aclexplode.grantor::regrole::name) AS grantor,
                    role_id(aclexplode.grantee::regrole::name) AS grantee,
                    aclexplode.privilege_type,
                    aclexplode.is_grantable,
                    c.relacl IS NULL AS "default"
                   FROM aclexplode(COALESCE(c.relacl, acldefault('r'::"char", c.relowner))) aclexplode(grantor, grantee, privilege_type, is_grantable)) acl ON true
          WHERE c.reltype <> 0::oid
        )
 SELECT id,
    grantor,
    grantee,
    privilege_type,
    is_grantable,
    "default",
    rn
   FROM dedup
  WHERE rn = 1
WITH DATA;

CREATE UNIQUE INDEX acl_mv_id_idx ON acl_mv USING btree (id, grantor, grantee, privilege_type);

CREATE OR REPLACE VIEW acl
AS WITH refresh AS (
         SELECT check_and_refresh_mv_once('acl_mv') AS refreshed
        )
 SELECT sub.id,
    sub.grantor,
    sub.grantee,
    sub.privilege_type,
    sub.is_grantable,
    sub."default"
   FROM acl_mv sub
     JOIN refresh ON true;