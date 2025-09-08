create table orders
(
    id         int primary key,
    product_id int references products (id)
    max_retries int
);

insert
into orders
values (1, 1,3);