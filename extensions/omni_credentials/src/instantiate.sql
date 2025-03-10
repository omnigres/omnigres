create function instantiate(schema regnamespace default 'omni_credentials',
                            env_var text default 'OMNI_CREDENTIALS_MASTER_PASSWORD') returns void
    language plpgsql
as
$$
begin
    perform set_config('search_path', schema::text || ',public', true);

    create table credentials_config
    (
        environment_variable text not null,
        unique (environment_variable)
    );

    insert
    into
        credentials_config (environment_variable)
    values (env_var);

    create type credential_kind as enum ('credential','api_key','api_secret','password', 'token', 'token[jwt]', 'token[oauth]', 'ssh','certificate', 'otp', 'saml_assertion');

    create table encrypted_credentials
    (
        name      text            not null,
        value     bytea           not null,
        kind      credential_kind not null default 'credential',
        principal regrole         not null default current_user::regrole,
        scope     jsonb           not null default '{
          "all": true
        }'
    );
    alter table encrypted_credentials
        enable row level security;
    create policy encrypted_credentials_principal on encrypted_credentials
        using (pg_has_role(current_user, principal, 'USAGE'));

    create unique index encrypted_credentials_uniq
        on encrypted_credentials (name, kind, principal, scope);

    create function decrypt_credential(credential bytea) returns text
        security definer
    begin
        atomic
        select
            pgp_sym_decrypt(credential, value)
        from
            omni_os.env
            inner join credentials_config on true
        where
            variable = environment_variable;
    end;

    create function encrypt_credential(credential text) returns bytea
        security definer
    begin
        atomic
        select
            pgp_sym_encrypt(credential, value)
        from
            omni_os.env
            inner join credentials_config on true
        where
            variable = environment_variable;
    end;


    if current_setting('server_version_num')::int / 10000 > 14 then
        create view credentials with (security_barrier, security_invoker) as
            select
                name,
                decrypt_credential(value) as value,
                kind,
                principal,
                scope
            from
                encrypted_credentials;
    else
        create function _credentials_view()
            returns table
                    (
                        name      text,
                        value     text,
                        kind      credential_kind,
                        principal regrole,
                        scope     jsonb
                    )
            security invoker
        begin
            atomic
            select
                name,
                decrypt_credential(value) as value,
                kind,
                principal,
                scope
            from
                encrypted_credentials;
        end;
        create view credentials with (security_barrier) as
            select * from _credentials_view();
    end if;

    alter table credentials
        alter column kind set default 'credential';
    alter table credentials
        alter column principal set default current_user::regrole;
    alter table credentials
        alter column scope set default '{"all": true}';

    create function credentials_update() returns trigger
        security definer
        language plpgsql as
    $code$
    begin
        if tg_op = 'DELETE' then
            delete
            from
                encrypted_credentials
            where
                name = old.name and
                kind = old.kind and
                principal = old.principal and
                scope = old.scope;
            return old;
        end if;
        if old is distinct from null then
            update encrypted_credentials
            set
                value = encrypt_credential(new.value),
                name  = new.name,
                kind = new.kind,
                principal = new.principal,
                scope = new.scope
            where
                name = old.name and
                kind = old.kind and
                principal = old.principal and
                scope = old.scope;
        else
            insert
            into
                encrypted_credentials (name, value, kind, principal, scope)
            values
                (new.name, encrypt_credential(new.value), new.kind, new.principal,
                 new.scope);
        end if;
        return new;
    end;
    $code$;
    execute format('alter function credentials_update set search_path to %I,public', schema);

    create trigger credentials_update
        instead of update or insert or delete
        on credentials
        for each row
    execute function credentials_update();

end;
$$;
