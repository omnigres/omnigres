create function instantiate_file_store(fs anyelement, filename text, schema regnamespace default 'omni_credentials') returns void
    language plpgsql
as
$$
begin
    perform set_config('search_path', schema::text || ',public', true);

    if filename not like '/%' then
        filename := current_setting('data_directory') || '/' || filename;
    end if;

    create table if not exists credential_file_stores
    (
        filename text unique
    );

    create or replace function credential_file_store_reload(filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        file_contents text;
    begin
        if filename not like '/%' then
            filename := current_setting('data_directory') || '/' || filename;
        end if;
        
        select convert_from(
            omni_vfs.read(
                omni_var.get_session(
                    'fs',
                    null::omni_vfs.remote_fs
                ),
                filename
            ),
            'utf8'
        ) into file_contents;

        if file_contents is null then
            raise exception 'File does not exist: %', filename;
        end if;

        create temp table __new_encrypted_credentials__
        (
            like encrypted_credentials
        ) on commit drop;

        insert into __new_encrypted_credentials__ (name, value, kind, principal, scope)
        select
            cred->>'name' AS name,
            decode(cred->>'value', 'base64') AS value,
            coalesce(
                nullif(cred->>'kind', '')::credential_kind,
                'credential'::credential_kind
            ) AS kind,
            coalesce(
                nullif(cred->>'principal', '')::regrole, 
                current_user::regrole
            ) AS principal,
            coalesce(
                (cred->>'scope')::jsonb,
                '{"all": true}'::jsonb
            ) AS scope
        from jsonb_array_elements(file_contents::jsonb) AS cred;


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

    insert into credential_file_stores (filename) values (instantiate_file_store.filename);

    perform credential_file_store_reload(filename);

    create or replace function update_credentials_file(filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        r record;
        json_content jsonb;
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

        perform omni_vfs.write(
            omni_var.get_session('fs', null::omni_vfs.remote_fs),
            filename,
            convert_to(json_content::text, 'utf8')
        );

        return TRUE;
    end;
    $code$;

    perform update_file(filename);

    create or replace function file_store_credentials_update() returns trigger
        security definer
        language plpgsql as
    $code$
    declare
        rec record;
    begin
        for rec in select * from credential_file_stores
            loop
                perform update_file(rec.filename);
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
