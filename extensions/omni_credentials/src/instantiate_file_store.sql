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
        fs_type text,
        fs_id integer,
        fs_mount text,
        connstr text,
        constructor text
    );

    create or replace function register_file_store(fs anyelement, filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        fs_id int;
        connstr text;
        constructor text;
        fs_type text;
        fs_mount text;
    begin
        fs_type := format_type(pg_typeof(fs), NULL);

        if fs_type = 'omni_vfs.local_fs' or fs_type = 'omni_vfs.table_fs' then
            execute 'select ($1).id' into fs_id using fs;
        end if;

        if fs_type = 'omni_vfs.local_fs' then
            execute 'select mount from omni_vfs.local_fs_mounts where id = ($1).id' into fs_mount using fs;
        end if;

        if fs_type = 'omni_vfs.remote_fs' then
            execute 'select ($1).connstr' into connstr using fs;
            execute 'select ($1).constructor' into constructor using fs;
        end if;

        insert into credential_file_stores (
            filename, 
            fs_type, 
            fs_id,
            fs_mount,
            connstr, 
            constructor
        ) values (
            filename,
            fs_type,
            fs_id,
            fs_mount,
            connstr,
            constructor
        );

        return true;
    end;
    $code$;

    perform register_file_store(fs, filename);

    create or replace function credential_file_store_reload(filename text) returns boolean
        language plpgsql
    as
    $code$
    declare
        file_contents text;
        fs_type text;
        fs_id int;
        fs_mount text;
        connstr text;
        constructor text;
    begin
        if filename not like '/%' then
            filename := current_setting('data_directory') || '/' || filename;
        end if;

        select 
            credential_file_stores.fs_type, 
            credential_file_stores.fs_id, 
            credential_file_stores.fs_mount, 
            credential_file_stores.connstr, 
            credential_file_stores.constructor 
        into fs_type, fs_id, fs_mount, connstr, constructor
        from credential_file_stores 
        where credential_file_stores.filename = credential_file_store_reload.filename;


        if fs_type = 'omni_vfs.local_fs' then
            select convert_from(
                omni_vfs.read(
                    omni_vfs.local_fs(fs_mount),
                    filename
                ),
                'utf8'
            ) into file_contents;
        elsif fs_type = 'omni_vfs.table_fs' then
            select convert_from(
                omni_vfs.read(
                    omni_vfs.table_fs(fs_id),
                    filename
                ),
                'utf8'
            ) into file_contents;
        elsif fs_type = 'omni_vfs.remote_fs' then
            select convert_from(
                omni_vfs.read(
                    omni_vfs.remote_fs(connstr, constructor),
                    filename
                ),
                'utf8'
            ) into file_contents;
        else
            raise exception 'Unknown filesystem type: %', fs_type;
        end if;

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
        r record;
        json_content jsonb;
        fs_type text;
        fs_id int;
        fs_mount text;
        connstr text;
        constructor text;
    begin
        select 
            credential_file_stores.fs_type, 
            credential_file_stores.fs_id, 
            credential_file_stores.fs_mount, 
            credential_file_stores.connstr, 
            credential_file_stores.constructor 
        into fs_type, fs_id, fs_mount, connstr, constructor
        from credential_file_stores 
        where credential_file_stores.filename = update_credentials_file.filename;

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

        if fs_type = 'omni_vfs.local_fs' then
            perform omni_vfs.write(
                omni_vfs.local_fs(fs_mount),
                filename,
                convert_to(json_content::text, 'utf8')
            );
        elsif fs_type = 'omni_vfs.table_fs' then
            perform omni_vfs.write(
                omni_vfs.table_fs(fs_id),
                filename,
                convert_to(json_content::text, 'utf8')
            );
        elsif fs_type = 'omni_vfs.remote_fs' then
            perform omni_vfs.write(
                omni_vfs.remote_fs(connstr, constructor),
                filename,
                convert_to(json_content::text, 'utf8')
            );
        else
            raise exception 'Unknown filesystem type: %', fs_type;
        end if;

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
