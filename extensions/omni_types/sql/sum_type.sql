\pset null 'NULL'
\set dump_types 'select typ as typname,variants from omni_types.sum_types'

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

-- Determining variant
begin;
select omni_types.sum_type('sum_type', 'integer', 'boolean');
\dT;
:dump_types;

select omni_types.variant(100::sum_type);
select omni_types.variant(true::sum_type);

-- Invalid type
select omni_types.variant(10);

rollback;

-- TOASTing

begin;
select omni_types.sum_type('sum_type', 'text', 'integer');
\dT;
:dump_types;

create table test (val sum_type);
alter table test alter val set storage external;

insert into test values (repeat('a', 100000)::text::sum_type),(1000);
select omni_types.variant(val) from test;
select (case when omni_types.variant(val) = 'text'::regtype then length(val::text)
             when omni_types.variant(val) = 'integer'::regtype then val::integer
             else null end) from test;

rollback;

-- Pseudo-types can't be used
begin;
select omni_types.sum_type('sum_type', 'text', 'anyarray');
rollback;

-- Binary I/O
begin;
select omni_types.sum_type('sum_type', 'text', 'integer');

select typsend = 'sum_type_send'::regproc as valid_send,
       typreceive = 'sum_type_recv'::regproc as valid_recv
from pg_type
where oid = 'sum_type'::regtype;

select sum_type_recv(sum_type_send('text(Hello)'));
select sum_type_recv(sum_type_send('integer(1000)'));

-- Malformed input
savepoint try;
select sum_type_recv(E''::bytea);
rollback to savepoint try;
select sum_type_recv(int4send(1));
rollback to savepoint try;
select sum_type_recv(int4send(2));
rollback to savepoint try;

rollback;

-- Adding variants
begin;
select omni_types.sum_type('sum_type', 'boolean', 'bigint');
\dT
:dump_types;

select omni_types.add_variant('sum_type', 'integer');
:dump_types;

select 'integer(1000)'::sum_type;
select 'bigint(1000)'::sum_type;
select 'boolean(t)'::sum_type;

rollback;

-- Adding variants to variable sized type
begin;
select omni_types.sum_type('sum_type', 'boolean', 'text');
\dT
:dump_types;

select omni_types.add_variant('sum_type', 'bigint');
:dump_types;

select 'text(test)'::sum_type;
select 'bigint(1000)'::sum_type;
select 'boolean(t)'::sum_type;

rollback;

-- Adding larger (invalid) variants to fixed sized type
begin;
select omni_types.sum_type('sum_type', 'boolean');
\dT
:dump_types;

select omni_types.add_variant('sum_type', 'bigint');

rollback;

-- Adding duplicate variants
begin;
select omni_types.sum_type('sum_type', 'integer');
\dT
:dump_types;

-- Using a different name for the same type here intentionally
-- to make sure it is checked against OIDs
select omni_types.add_variant('sum_type', 'int4');

rollback;

-- Sum type with unit
begin;
create domain ok as omni_types.unit;
create domain result as integer;
select omni_types.sum_type('sum_type', 'ok', 'result');
\dT
:dump_types;

select sum_type_from_ok(omni_types.unit());
end;

-- Ensure no types are leaked
\dT;
:dump_types;