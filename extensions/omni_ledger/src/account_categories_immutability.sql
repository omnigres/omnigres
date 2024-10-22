create or replace function account_categories_immutability() returns trigger as
$$
begin
    if new.type != old.type then
        raise exception 'cannot change the account category type once it has been set';
    end if;
    if new.debit_normal != old.debit_normal then
        raise exception 'cannot change account category debit normalness once it has been set';
    end if;
    return new;
end;
$$ language plpgsql;