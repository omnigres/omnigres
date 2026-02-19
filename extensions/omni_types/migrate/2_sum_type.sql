create table sum_types
(
    typ      regtype primary key unique,
    variants regtype[] not null
);

/*{% include "../src/sum_type_unique_variants_trigger_func.sql" %}*/

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

create function rename(type_to_rename regtype, new_name name) returns void
    language plpgsql as
$$
declare
    rec  record;
    vrec record;
begin
    -- Rename sum types
    for rec in select typ, variants from omni_types.sum_types where typ = type_to_rename
        loop
            execute format('alter function %I_in rename to %I_in', rec.typ, new_name);
            execute format('alter function %I_out rename to %I_out', rec.typ, new_name);
            execute format('alter function %I_recv rename to %I_recv', rec.typ, new_name);
            execute format('alter function %I_send rename to %I_send', rec.typ, new_name);
            for vrec in select
                            typname
                        from
                            lateral unnest(rec.variants) as variant
                            inner join pg_type on pg_type.oid = variant
                loop
                    execute format('alter function %I rename to %I', rec.typ || '_from_' || vrec.typname,
                                   new_name || '_from_', vrec.typname);
                    execute format('alter function %I rename to %I', vrec.typname || '_from_' || rec.typ,
                                   vrec.typname || '_from_', new_name);
                end loop;
            execute format('alter type %I rename to %I', rec.typ, new_name);
        end loop;
    return;
end
$$;