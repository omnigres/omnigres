create table users
(
    id     integer primary key generated always as identity,
    name   text not null,
    email  text not null unique,
    locked bool not null default false
);

create table products
(
    id     integer primary key generated always as identity,
    name   text    not null,
    listed boolean not null default false
);

create table products_owners
(
    product_id integer not null references products (id),
    user_id    integer not null references users (id)
);

create or replace function current_user_role() returns text
    language 'sql' as
$$
select 'registered_user'
$$ stable;
create or replace function current_user_privileged() returns boolean
    language 'sql' as
$$
select false
$$ stable;

-- Data
with new_users
         as (insert into users (name, email) values ('Alice', 'alice@foobar.com'), ('Bob', 'bob@foobar.com') returning id, name),
     new_products as (insert into products (name, listed) values ('Quantum computer', false),
                                                                 ('Bar of soap', false),
                                                                 ('Car', true) returning id, name)
insert
into products_owners (product_id, user_id)
values ((select id from new_products where name = 'Quantum computer'), (select id from new_users where name = 'Alice'));

-- Current user
create or replace function current_user_id() returns integer
    language 'sql'
    security definer as
$$
select id
from public.users
where name = 'Alice'
$$ stable;
