create table sum_types
(
    typ      regtype primary key unique,
    variants regtype[] not null
);

create or replace function sum_type_unique_variants_trigger_func()
    returns trigger as
$$
declare
    duplicate_count integer;
begin
    select
        count(*) - count(distinct element)
    into duplicate_count
    from
        unnest(new.variants) as element;

    if duplicate_count > 0 then
        raise exception 'Sum types can not contain duplicate variants';
    end if;
    return new;
end;
$$ language plpgsql;

create trigger sum_type_unique_variants_trigger
    before insert or update
    on sum_types
    for each row
execute function sum_type_unique_variants_trigger_func();

create function sum_type(name name,
                         variadic variants regtype[]
) returns regtype
as
'MODULE_PATHNAME',
'sum_type'
    language c;

create function add_variant(sum_type regtype, variant regtype) returns void
as
'MODULE_PATHNAME',
'add_variant'
    language c;

create function variant(v anycompatible) returns regtype as
'MODULE_PATHNAME',
'sum_variant' language c;