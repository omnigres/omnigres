create function instantiate_file_store(fs anyelement, filename text, schema regnamespace default 'omni_credentials') returns void
    language plpgsql
as
$$
declare
    vfs_type text;
    vfs_info jsonb;
begin
    perform set_config('search_path', schema::text || ',public', true);

    -- Determine VFS type and info
    vfs_type := pg_typeof(fs)::text;
    if vfs_type = 'omni_vfs.local_fs' then
        vfs_info := jsonb_build_object('mount', (fs).mount);
    elsif vfs_type = 'omni_vfs.table_fs' then
        vfs_info := jsonb_build_object('name', (fs).name);
    elsif vfs_type = 'omni_vfs.remote_fs' then
        vfs_info := jsonb_build_object('connstr', (fs).connstr, 'constructor', (fs).constructor);
    else
        raise exception 'Unsupported VFS type: %', vfs_type;
    end if;

    create table if not exists credential_file_stores
    (
        filename text unique,
        vfs_type text not null,
        vfs_info jsonb not null
    );

    insert into credential_file_stores (filename, vfs_type, vfs_info)
        values (filename, vfs_type, vfs_info)
        on conflict (filename) do update set vfs_type = excluded.vfs_type, vfs_info = excluded.vfs_info;

    -- Create or replace reload function
    create or replace function credential_file_store_reload(filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        rec record;
        file_contents_bytea bytea;
        file_contents text;
        fs anyelement;
    begin
        select vfs_type, vfs_info into rec from credential_file_stores where filename = credential_file_store_reload.filename;
        if not found then
            raise exception 'No file store registered for filename: %', filename;
        end if;
        -- Reconstruct fs
        if rec.vfs_type = 'omni_vfs.local_fs' then
            fs := omni_vfs.local_fs(rec.vfs_info->>'mount');
        elsif rec.vfs_type = 'omni_vfs.table_fs' then
            fs := omni_vfs.table_fs(rec.vfs_info->>'name');
        elsif rec.vfs_type = 'omni_vfs.remote_fs' then
            fs := omni_vfs.remote_fs(rec.vfs_info->>'connstr', rec.vfs_info->>'constructor');
        else
            raise exception 'Unsupported VFS type: %', rec.vfs_type;
        end if;
        file_contents_bytea := omni_vfs.read(fs, filename);
        file_contents := convert_from(file_contents_bytea, 'utf8');
        if file_contents is null then
            raise exception 'File does not exist: %', filename;
        end if;
        create temp table __new_encrypted_credentials__
        (
            like encrypted_credentials
        ) on commit drop;
        insert into __new_encrypted_credentials__ (name, value, kind, principal, scope)
        select
            nullif(cred->>'name', '') as name,
            decode(cred->>'value', 'base64') as value,
            nullif(cred->>'kind', '')::credential_kind as kind,
            nullif(cred->>'principal', '')::regrole as principal,
            nullif(cred->>'scope','')::jsonb as scope
        from jsonb_array_elements(file_contents::jsonb) as cred;
        insert into encrypted_credentials (name, value, kind, principal, scope)
        select name, value, kind, principal, scope
        from __new_encrypted_credentials__
        on conflict (name, kind, principal, scope) do update set value = excluded.value;
        return true;
    exception
        when others then return false;
    end;
    $code$;
    execute format('alter function credential_file_store_reload set search_path to %I,public', schema);

    perform credential_file_store_reload(filename);

    -- Create or replace update function
    create or replace function update_credentials_file(filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        rec record;
        json_content jsonb;
        fs anyelement;
    begin
        select vfs_type, vfs_info into rec from credential_file_stores where filename = update_credentials_file.filename;
        if not found then
            raise exception 'No file store registered for filename: %', filename;
        end if;
        -- Reconstruct fs
        if rec.vfs_type = 'omni_vfs.local_fs' then
            fs := omni_vfs.local_fs(rec.vfs_info->>'mount');
        elsif rec.vfs_type = 'omni_vfs.table_fs' then
            fs := omni_vfs.table_fs(rec.vfs_info->>'name');
        elsif rec.vfs_type = 'omni_vfs.remote_fs' then
            fs := omni_vfs.remote_fs(rec.vfs_info->>'connstr', rec.vfs_info->>'constructor');
        else
            raise exception 'Unsupported VFS type: %', rec.vfs_type;
        end if;
        select jsonb_agg(
            jsonb_build_object(
                'name', name,
                'value', replace(encode(value, 'base64'), E'\n', ''),
                'kind', kind,
                'principal', principal,
                'scope', scope
            )
        )
        into json_content
        from encrypted_credentials;
        perform omni_vfs.write(fs, filename, convert_to(json_content::text, 'utf8'), create_file => true);
        return true;
    end;
    $code$;
    execute format('alter function update_credentials_file set search_path to %I,public', schema);

    perform update_credentials_file(filename);

    -- Create or replace trigger function
    create or replace function file_store_credentials_update() returns trigger
        security definer
        language plpgsql as
    $code$
    declare
        rec record;
    begin
        for rec in select filename from credential_file_stores
            loop
                perform update_credentials_file(rec.filename);
            end loop;
        return new;
    end;
    $code$;
    execute format('alter function file_store_credentials_update set search_path to %I,public', schema);

    -- Create trigger if not exists
    perform from pg_trigger where tgname = 'file_store_credentials_update' and tgrelid = 'encrypted_credentials'::regclass;
    if not found then
        create constraint trigger file_store_credentials_update
            after update or insert or delete
            on encrypted_credentials
            deferrable initially deferred
            for each row
        execute function file_store_credentials_update();
    end if;
end;
$$;
