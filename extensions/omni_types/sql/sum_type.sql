\pset null 'NULL'
\set dump_types 'select typname,variants from omni_types.sum_types st inner join pg_type t on t.oid = st.oid'

create or replace function get_type_size(data_type regtype)
    returns integer as
$$
select
    typlen
from
    pg_type
where
    oid = data_type
$$
    language sql;


-- Empty
begin;

select omni_types.sum_type('empty', variadic array []::regtype[]);
\dT
:dump_types;

select null::empty is null as is_null;

rollback;

-- Single fixed size by val
begin;
select omni_types.sum_type('sum_type', 'integer');
\dT;
:dump_types;

select get_type_size('sum_type');

select 'integer(100)'::sum_type;
select 'integer(100)'::sum_type::integer;
select 100::sum_type;

rollback;

-- Multiple fixed size by val
begin;
select omni_types.sum_type('sum_type', 'integer', 'bigint');
\dT;
:dump_types;

select get_type_size('sum_type');

select 'integer(1000)'::sum_type;
select 'integer(1000)'::sum_type::integer;
select 1000::sum_type;
select 'bigint(10000000000)'::sum_type;
select 'bigint(10000000000)'::sum_type::bigint;
select 10000000000::bigint::sum_type;

rollback;

-- Single fixed size
begin;
select omni_types.sum_type('sum_type', 'name');
\dT;
:dump_types;

select get_type_size('sum_type');

select 'name(test)'::sum_type;
select 'name(test)'::sum_type::name;
select 'test'::name::sum_type;

rollback;

-- Multiple fixed size, by val is mixed
begin;
select omni_types.sum_type('sum_type', 'name', 'integer');
\dT;
:dump_types;

select get_type_size('sum_type');

select 'name(test)'::sum_type;
select 'name(test)'::sum_type::name;
select 'test'::name::sum_type;
select 'integer(1000)'::sum_type;
select 'integer(1000)'::sum_type::integer;
select 1000::sum_type;

rollback;

-- Single variable size
begin;
select omni_types.sum_type('sum_type', 'text');
\dT;
:dump_types;

select get_type_size('sum_type');

select 'text(Hello)'::sum_type;
select 'text(Hello)'::sum_type::text;
select 'Hello'::text::sum_type;

rollback;

-- Multiple mixed variable and fixed size
begin;
select omni_types.sum_type('sum_type', 'text', 'integer', 'name');
\dT;
:dump_types;

select get_type_size('sum_type');

select 'text(Hello)'::sum_type;
select 'text(Hello)'::sum_type::text;
select 'Hello'::text::sum_type;
select 'integer(1000)'::sum_type;
select 'integer(1000)'::sum_type::integer;
select 1000::sum_type;
select 'name(test)'::sum_type;
select 'name(test)'::sum_type::name;
select 'test'::name::sum_type;

rollback;

-- Domains
begin;
create domain height as integer check (value > 0);
create domain age as integer check (value > 0 and value < 200);

select omni_types.sum_type('sum_type', 'height', 'age');
\dT;
:dump_types;

select get_type_size('sum_type');

select 'height(100)'::sum_type;
select 'age(100)'::sum_type;

-- Can't cast domain types, use functions
select sum_type_from_height(100);
select sum_type_from_age(100);

rollback;

--- Composite types
begin;
create type person as
(
    name text,
    dob  date
);

create type animal as enum ('dog', 'cat', 'fish', 'other');

create type pet as
(
    name   text,
    animal animal
);

select omni_types.sum_type('sum_type', 'person', 'pet');
\dT;
:dump_types;

select get_type_size('sum_type');

select $$person((John,01/01/1980))$$::sum_type;
select $$pet((Charlie,dog))$$::sum_type;
select row ('John', '01/01/1980')::person::sum_type;
select row ('Charlie','dog')::pet::sum_type;
select row ('John', '01/01/1980')::person::sum_type::person;
select row ('John', '01/01/1980')::person::sum_type::pet;
select row ('Charlie','dog')::pet::sum_type::pet;
select row ('Charlie','dog')::pet::sum_type::person;


rollback;

-- Casting variants to different types
begin;
select omni_types.sum_type('sum_type', 'integer', 'boolean');
\dT;
:dump_types;

select 100::sum_type::boolean;
select true::sum_type::integer;

rollback;

-- Duplicates
begin;
select omni_types.sum_type('sum_type', 'integer', 'integer');

rollback;

-- Ensure no types are leaked
\dT;
:dump_types;