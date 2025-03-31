create table person (
    id int primary key generated always as identity,
    name text not null,
    email text not null
);
