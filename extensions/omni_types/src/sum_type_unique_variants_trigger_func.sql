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