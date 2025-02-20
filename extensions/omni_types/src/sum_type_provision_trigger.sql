create function sum_type_provision_trigger() returns trigger
    language plpgsql
as
$$
begin
    -- Hash?
    perform from pg_proc where proname = new.typ || '_hash' and pronamespace = current_schema()::regnamespace;

    if found then
        execute format('create operator family %I using hash', new.typ || '_hash_ops');

        execute format('create operator class %I
        default for type %2$I using hash
        family %3$I as
        operator 1 =,
        function 1 %4$I(%2$I)', new.typ || '_hash_ops_class', new.typ, new.typ || '_hash_ops', new.typ || '_hash');
    end if;

    -- Btree?
    perform from pg_proc where proname = new.typ || '_cmp' and pronamespace = current_schema()::regnamespace;

    if found then
        execute format('create operator family %I using btree', new.typ || '_btree_ops');

        execute format('create operator class %I
        default for type %2$I using btree
        family %3$I as
        operator 1 <,
        operator 2 <=,
        operator 3 =,
        operator 4 >=,
        operator 5 >,
        function 1 %4$I(%2$I,%2$I)', new.typ || '_btree_ops_class', new.typ, new.typ || '_btree_ops',
                       new.typ || '_cmp');
    end if;

    return new;
end;
$$;
