create function instantiate(schema regnamespace default 'omni_audit',
                            auditor_role name default 'omni_audit',
                            owner_role name default 'omni_audit'
) returns void
    language plpgsql
as
$$
declare
    old_search_path text := current_setting('search_path');
begin
    -- Set the search path to target schema and public
    perform
        set_config('search_path', schema::text || ',public', true);

    -- Setup roles
    perform from pg_roles where rolname = auditor_role;
    if not found then
        execute format('create role %I', auditor_role);
    end if;
    perform from pg_roles where rolname = owner_role;
    if not found then
        execute format('create role %I', owner_role);
    end if;


    -- Register of commands
    create type operation_type as enum ('INSERT', 'UPDATE', 'DELETE', 'TRUNCATE');

    create table tracked_transaction
    (
        transaction_id bigint not null primary key generated always as identity
    );

    create table tracked_operation
    (
        operation_id        bigint                 not null primary key generated always as identity,
        transaction_id      bigint                 not null references tracked_transaction (transaction_id),
        operation_type operation_type not null,
        operation_statement text                   not null,
        performed_at        pg_catalog.timestamptz not null default now()
    );

    -- Register of tracked relations
    create table tracked_relation
    (
        id       bigint primary key generated always as identity,
        relation regclass not null unique
    );

    create function as_of(as_of_time timestamptz, local boolean default true) returns timestamptz
    begin
        atomic;
        select set_config('omni_audit.as_of', as_of_time::text, local)::timestamptz;
    end;


    create function track_relation(operation_type operation_type, operation_statement text) returns bigint
        language plpgsql
    as
    $track_relation$
    declare
        txid_ text := current_setting('omni_audit.txid', true);
        txid bigint;
        opid bigint;
    begin
        case
            when txid_ is null or txid_ = '' then txid := nextval('tracked_transaction_transaction_id_seq');
                                                  perform set_config('omni_audit.txid', txid::text, true);
            else txid := txid_::bigint; end case;

        insert
        into
            tracked_transaction (transaction_id) overriding system value
        values (txid)
        on conflict do nothing;

        insert
        into
            tracked_operation (transaction_id, operation_type, operation_statement)
        select
            txid,
            operation_type,
            operation_statement
        returning operation_id into opid;

        return opid;
    end;
    $track_relation$;
    execute format(
            'alter function track_relation set search_path to %I,public',
            schema);


    create function track_relation_insert() returns trigger
        language plpgsql
    as
    $track_relation_insert$
    begin
        perform track_relation(tg_op::operation_type, current_query());
        execute format('insert into %1$I.%2$I (select * from %)', tg_table_schema, tg_table_name, new);
        return new;
    end;

    $track_relation_insert$;
    execute format(
            'alter function track_relation_insert set search_path to %I,public',
            schema);


    create function tracked_relation_change() returns trigger
        language plpgsql
    as
    $tracked_relation_change$
    declare
        rel          record;
        opid bigint;
        pkeys        text;
        r_pkeys      text;
        oldrel_pkeys text;
        updated_pkeys text;
    begin
        for rel in select
                       r.relation as relation,
                       ns.nspname as namespace,
                       c.oid      as relid,
                       ct.conkey  as pkeys
                   from
                       newrel                   r
                       left join  pg_trigger    tg on tg.tgrelid = r.relation and
                                                      tg.tgfoid = 'track_relation_insert'::regproc::oid
                       inner join pg_class      c on c.oid = r.relation
                       inner join pg_namespace  ns on ns.oid = c.relnamespace
                       left join  pg_constraint ct on ct.contype = 'p' and ct.conrelid = c.oid
                   where
                       tg.oid is null
            loop
                if cardinality(rel.pkeys) > 0 then
                    select
                        '(' || string_agg(a.attname, ',') || ')',
                        '(' || string_agg('r.' || a.attname, ',') || ')',
                        '(' || string_agg('oldrel.' || a.attname, ',') || ')',
                        '(' || string_agg('updated.' || a.attname, ',') || ')'
                    from
                        pg_attribute                                                            a
                        inner join (select i from generate_series(1, cardinality(rel.pkeys)) i) k on true
                    where
                        attrelid = rel.relid and
                        attnum = rel.pkeys[k.i]
                    into pkeys, r_pkeys, oldrel_pkeys, updated_pkeys;
                else
                    raise exception 'auditing relations without primary keys is currently unsupported';
                end if;
                execute format($audit_tab$
            create table %1$I.%3$I (
             like %1$I.%2$I excluding constraints,
             "(operation_id)" bigint not null,
             "(valid)" tstzrange not null default tstzrange(statement_timestamp(), 'infinity'),
             exclude using gist (%9$s with =, "(valid)" with &&)
            );
            create view %1$I.%11$I as
             with env as (select (case when current_setting('omni_audit.as_of', true) = '' then null else current_setting('omni_audit.as_of', true) end)::timestamptz as as_of)
             select * from env
               left join %1$I.%3$I on true
              where env.as_of is null or ("(valid)" @> env.as_of);
            create function %4$I() returns trigger
            language plpgsql
            security definer
            as
            $%4$I$
            declare
              opid bigint;
            begin
                opid := track_relation(tg_op::operation_type, current_query());
                insert into %1$I.%3$I select r.*, opid from newrel r;
                return new;
            end;
            $%4$I$;
            alter function %4$I set search_path to %6$I,public;
            create function %5$I() returns trigger
            language plpgsql
            security definer
            as
            $%5$I$
            declare
              opid bigint;
              ts timestamptz := statement_timestamp();
            begin
                opid := track_relation(tg_op::operation_type, current_query());
                with updated as (update %1$I.%3$I r set "(valid)" = tstzrange(lower("(valid)"), ts)
                   from oldrel where %7$s = %8$s and upper("(valid)") = 'infinity' returning r.*)
                insert into %1$I.%3$I (select r.*, opid from newrel r inner join updated on %7$s = %10$s);
                return new;
            end;
            $%5$I$;
            alter function %5$I set search_path to %6$I,public;
            $audit_tab$, rel.namespace, rel.relation, 'audited(' || rel.relation || ')',
                               'track_relation_insert_' || rel.relation,
                               'track_relation_update_' || rel.relation,
                               current_schema, r_pkeys, oldrel_pkeys, pkeys, updated_pkeys,
                               'audited_view(' || rel.relation || ')');
                opid := track_relation(tg_op::operation_type, current_query());
                execute format($populate_audit_tab$
            insert into %1$I (select %2$I.*, %3$L::bigint from %2$I)
                $populate_audit_tab$, 'audited(' || rel.relation || ')', rel.relation, opid);
            end loop;
        execute format(
                'create trigger %1$I after insert on %2$I referencing new table as newrel for each statement execute function %1$I()',
                'track_relation_insert_' || rel.relation, rel.relation);
        execute format(
                'create trigger %1$I after update on %2$I referencing new table as newrel old table as oldrel for each statement execute function %1$I()',
                'track_relation_update_' || rel.relation, rel.relation);
        return new;
    end;
    $tracked_relation_change$;

    execute format(
            'alter function tracked_relation_change set search_path to %I,public',
            schema);

    create trigger tracked_relation_change
        after insert
        on tracked_relation
        referencing new table as newrel
        for each statement
    execute function tracked_relation_change();


    -- Track `tracked_relation` to ensure we know what's been tracked
    insert
    into
        tracked_relation (relation)
    values
        ('tracked_relation');

    execute format('alter schema %I owner to %I', schema, owner_role);
    execute format('revoke all on schema %I from public', schema);
    execute format('grant usage on schema %I to public', schema);
    execute format('grant insert,delete on table %I.tracked_relation to %I', schema, owner_role);

    -- Restore the path
    perform set_config('search_path', old_search_path, true);
end;
$$;
