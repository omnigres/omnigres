create function instantiate(schema regnamespace default 'omni_differential') returns void
    language plpgsql
as
$instantiate$
declare
    old_search_path text := current_setting('search_path');
begin
    perform
        set_config('search_path', schema::text || ',public', true);

    CREATE TYPE omni_snapshot AS (observer_xid xid8, command_id bigint, snapshot pg_snapshot);

    create function current_cid() returns bigint
        as 'MODULE_PATHNAME' language c;

    create function diff_def(sql text, fun_name text) returns text
        as 'MODULE_PATHNAME' language c;

    create function create_diff(sql text, fun_name text) returns void as
    $$ BEGIN
        EXECUTE omni_differential.diff_def(sql, fun_name);
    END $$ language plpgsql;

    create function diff_def_body(sql text) returns text
        as 'MODULE_PATHNAME' language c;

    create function invoke_diff(sql text, snapshot omni_differential.omni_snapshot) returns setof record as
    $$ BEGIN
        RETURN QUERY EXECUTE omni_differential.diff_def_body(sql) USING snapshot;
    END $$ language plpgsql;

    CREATE FUNCTION current_snapshot() RETURNS omni_snapshot
        RETURN ROW(pg_current_xact_id(), omni_differential.current_cid(), pg_current_snapshot());

    create table saved_snapshots (name text primary key, snap omni_snapshot);

    create function save_snapshot(name text) returns void
    begin atomic
        insert into saved_snapshots values (name, current_snapshot())
        on conflict (name) do update set snap = excluded.snap;
    end;

    create function fetch_snapshot(name_ text) returns omni_snapshot
    begin atomic
        select snap from saved_snapshots ss where ss.name = name_;
    end;

    -- Is an effect (row insertion or deletion) visible in the given omni_snapshot? -- TODO: consider null safety
    CREATE FUNCTION is_visible(effect_txid xid8, effect_cid bigint, snap omni_snapshot) RETURNS boolean AS
    $$ BEGIN
        IF effect_txid = snap.observer_xid THEN
            RETURN effect_cid < snap.command_id;
        END IF;
        RETURN pg_visible_in_snapshot(effect_txid, snap.snapshot);
    END $$ LANGUAGE plpgsql;

    create schema if not exists omni_history;

    CREATE FUNCTION trigger_on_insert() RETURNS trigger AS
    $$ BEGIN
        EXECUTE format('INSERT INTO omni_history.%I SELECT i::%I.%I, $1, $2 FROM inserted i',
            TG_ARGV[0], TG_TABLE_SCHEMA, TG_TABLE_NAME) USING pg_current_xact_id(), omni_differential.current_cid();
        RETURN NULL;
    END $$ LANGUAGE plpgsql;

    CREATE FUNCTION trigger_on_delete() RETURNS trigger AS
    $$ BEGIN
        EXECUTE format('INSERT INTO omni_history.%I SELECT d::%I.%I, $1, $2 FROM deleted d',
            TG_ARGV[0], TG_TABLE_SCHEMA, TG_TABLE_NAME) USING pg_current_xact_id(), omni_differential.current_cid();
        RETURN NULL;
    END $$ LANGUAGE plpgsql;

    CREATE FUNCTION trigger_on_update() RETURNS trigger AS
    $$ BEGIN
        EXECUTE format('INSERT INTO omni_history.%I SELECT i::%I.%I, $1, $2 FROM inserted i',
            TG_ARGV[0], TG_TABLE_SCHEMA, TG_TABLE_NAME) USING pg_current_xact_id(), omni_differential.current_cid();
        EXECUTE format('INSERT INTO omni_history.%I SELECT d::%I.%I, $1, $2 FROM deleted d',
            TG_ARGV[1], TG_TABLE_SCHEMA, TG_TABLE_NAME) USING pg_current_xact_id(), omni_differential.current_cid();
        RETURN NULL;
    END $$ LANGUAGE plpgsql;

    CREATE FUNCTION create_or_replace_history_triggers(
        relation_schema text, relation_name text, hist_ins_name text, hist_del_name text
    ) RETURNS void AS
    $$ BEGIN
        EXECUTE format($trg$
            CREATE OR REPLACE TRIGGER omni_hist_on_insert AFTER INSERT ON %I.%I
                REFERENCING NEW TABLE AS inserted FOR EACH STATEMENT
                EXECUTE FUNCTION omni_differential.trigger_on_insert(%L);
        $trg$, relation_schema, relation_name, hist_ins_name);
        EXECUTE format($trg$
            CREATE OR REPLACE TRIGGER omni_hist_on_delete AFTER DELETE ON %I.%I
                REFERENCING OLD TABLE AS deleted FOR EACH STATEMENT
                EXECUTE FUNCTION omni_differential.trigger_on_delete(%L);
        $trg$, relation_schema, relation_name, hist_del_name);
        EXECUTE format($trg$
            CREATE OR REPLACE TRIGGER omni_hist_on_update AFTER UPDATE ON %I.%I
                REFERENCING NEW TABLE AS inserted OLD TABLE AS deleted FOR EACH STATEMENT
                EXECUTE FUNCTION omni_differential.trigger_on_update(%L, %L);
        $trg$, relation_schema, relation_name, hist_ins_name, hist_del_name);
    END $$ LANGUAGE plpgsql;

    CREATE FUNCTION get_or_assign_relation_uuid(rel_schema text, rel_name text) RETURNS text AS
    $$ DECLARE
        result text = obj_description((format('%I.%I', rel_schema, rel_name))::regclass, 'pg_class');
    BEGIN
        IF result IS NOT NULL THEN
            RETURN result;
        END IF;
        result := rel_name || '_' || rel_schema || '_' || gen_random_uuid();
        EXECUTE format('COMMENT ON TABLE %I.%I IS %L', rel_schema, rel_name, result);
        RETURN result;
    END;
    $$ LANGUAGE plpgsql;

    CREATE FUNCTION instrument_relation(relation_schema text, relation_name text) RETURNS void AS
    $instrument_relation$
    DECLARE
        -- rel_regclass regclass = (format('%I.%I', relation_schema, relation_name))::regclass;
        id text = omni_differential.get_or_assign_relation_uuid(relation_schema, relation_name);
        hist_ins_name text = format('%s_hist_ins', id);
        hist_del_name text = format('%s_hist_del', id);
        -- new? diff? not sure yet
        hist_new_ins_name text = format('%s_ins_since', id);
        hist_new_del_name text = format('%s_del_since', id);

        hist_gain_name text = format('%s_gain_since', id);
        hist_loss_name text = format('%s_loss_since', id);
    BEGIN
        EXECUTE format($$ CREATE TABLE omni_history.%I(row_value %I.%I, tx_id xid8, c_id bigint);$$,
                       hist_ins_name, relation_schema, relation_name);
        EXECUTE format($$ CREATE TABLE omni_history.%I(row_value %I.%I, tx_id xid8, c_id bigint);$$,
                       hist_del_name, relation_schema, relation_name);

        PERFORM omni_differential.create_or_replace_history_triggers(
            relation_schema, relation_name, hist_ins_name, hist_del_name
        );
        -- creates a parametrized sql function that returns the history table but only rows
        -- not visible to the given omni_snapshot
        EXECUTE format($$
            CREATE FUNCTION omni_history.%I(snapshot omni_snapshot) RETURNS SETOF %I.%I
            BEGIN ATOMIC
                SELECT ((h.row_value).*) FROM omni_history.%I AS h
                    WHERE NOT omni_differential.is_visible(h.tx_id, h.c_id, snapshot);
            END
            $$, hist_new_ins_name, relation_schema, relation_name, hist_ins_name);

        EXECUTE format($$
            CREATE FUNCTION omni_history.%I(snapshot omni_snapshot) RETURNS SETOF %I.%I
            BEGIN ATOMIC
                SELECT ((h.row_value).*) FROM omni_history.%I AS h
                    WHERE NOT omni_differential.is_visible(h.tx_id, h.c_id, snapshot);
            END
            $$, hist_new_del_name, relation_schema, relation_name, hist_del_name);

        EXECUTE format($calculate_gain$
            CREATE FUNCTION omni_history.%I(snapshot omni_snapshot) RETURNS SETOF %I.%I
            BEGIN ATOMIC
                SELECT omni_history.%I(snapshot) EXCEPT ALL SELECT omni_history.%I(snapshot);
            END
            $calculate_gain$, hist_gain_name, relation_schema, relation_name, hist_new_ins_name, hist_new_del_name);
        EXECUTE format($calculate_loss$
            CREATE FUNCTION omni_history.%I(snapshot omni_snapshot) RETURNS SETOF %I.%I
            BEGIN ATOMIC
                SELECT omni_history.%I(snapshot) EXCEPT ALL SELECT omni_history.%I(snapshot);
            END
            $calculate_loss$, hist_loss_name, relation_schema, relation_name, hist_new_del_name, hist_new_ins_name);

        END
    $instrument_relation$ LANGUAGE plpgsql;

-- Restore the path
    perform set_config('search_path', old_search_path, true);

--- temporary stuff for experimentation
    create table public.users (
                           id serial primary key,
                           name text,
                           year numeric not null
    );
    create table public.users2 (
                           id serial primary key,
                           name text,
                           year numeric not null
    );

    perform omni_differential.instrument_relation('public', 'users');
    perform omni_differential.instrument_relation('public', 'users2');

    create function omni_differential.insert_test_data() returns void language sql
    begin atomic
    insert into public.users (name, year) values
        ('Alice', 2000),
        ('Bob', 1998),
        ('Charlie', 2001);
    insert into public.users2 (name, year) values
        ('Alice2', 2001),
        ('Bob2', 1999),
        ('Charlie', 2001);
    end;

end
$instantiate$;
