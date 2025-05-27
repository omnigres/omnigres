create function instantiate_file_store(fs anyelement, filename text, schema regnamespace default 'omni_credentials') returns void
    language plpgsql
as
$$
begin
    perform set_config('search_path', schema::text || ',public', true);

    if filename not like '/%' then
        filename := '/' || filename;
    end if;

    create table if not exists credential_file_stores
    (
        filename text unique,
        vfs text,
        vfs_type regtype
    );
  
    insert into credential_file_stores (filename, vfs_type, vfs) values (instantiate_file_store.filename, pg_typeof(fs), fs::text);

    create or replace function credential_file_store_reload(filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        file_contents text;
        file_contents_bytea bytea;
        v_vfs text;
        v_vfs_type text;
    begin
        if filename not like '/%' then
            filename := current_setting('data_directory') || '/' || filename;
        end if;

        select vfs, vfs_type into v_vfs, v_vfs_type from credential_file_stores where credential_file_stores.filename = credential_file_store_reload.filename;

        execute format(
            'select omni_vfs.read(%1$L::%2$s, %3$L);',
            v_vfs,
            v_vfs_type,
            filename
        ) into file_contents_bytea;
        
        select convert_from(file_contents_bytea, 'utf8') into file_contents;

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

    create or replace function update_credentials_file(filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        json_content jsonb;
        v_vfs text;
        v_vfs_type text;
    begin
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

        select vfs, vfs_type into v_vfs, v_vfs_type from credential_file_stores where credential_file_stores.filename = update_credentials_file.filename;

        execute format(
          'select omni_vfs.write(%1$L::%2$s, %3$L, %4$L);',
          v_vfs,
          v_vfs_type,
          filename,
          json_content
        );

        return true;
    end;
    $code$;

    perform update_credentials_file(filename);

    create or replace function file_store_credentials_update() returns trigger
        security definer
        language plpgsql as
    $code$
    declare
        rec record;
    begin
        for rec in select * from credential_file_stores
            loop
                perform update_credentials_file(rec.filename);
            end loop;
        return new;
    end;
    $code$;
    execute format('alter function file_store_credentials_update set search_path to %I,public', schema);

    perform
    from pg_trigger
    where tgname = 'file_store_credentials_update'
      and tgrelid = 'encrypted_credentials'::regclass;

    if not found then
        create constraint trigger file_store_credentials_update
            -- TODO: truncate can't be supported at this level
            after update or insert or delete
            on encrypted_credentials
            deferrable initially deferred
            for each row
        execute function file_store_credentials_update();
    end if;

end;
$$;
