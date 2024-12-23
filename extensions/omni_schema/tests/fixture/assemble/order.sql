create table orders
(
    id         int primary key,
    product_id int references products (id)
);

insert
into orders
values (1, 1);