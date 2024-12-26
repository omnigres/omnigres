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

    insert into credentials_config (environment_variable) values (env_var);

    create table encrypted_credentials
    (
        name  text  not null unique,
        value bytea not null
    );

    create function credentials_master_password() returns text
        language sql
    as
    $sql$
    select value
    from omni_os.env
             inner join credentials_config on true
    where variable = environment_variable
    $sql$;
    execute format('alter function credentials_master_password set search_path to %I', schema);


    create view credentials as
    select name,
           pgp_sym_decrypt(value, credentials_master_password()) as value
    from encrypted_credentials;

    create function credentials_update() returns trigger
        security definer
        language plpgsql as
    $code$
    begin
        if old is distinct from null then
            update encrypted_credentials
            set value = pgp_sym_encrypt(new.value, credentials_master_password()),
                name  = new.name
            where name = old.name;
        else
            insert
            into encrypted_credentials (name, value)
            values (new.name, pgp_sym_encrypt(new.value, credentials_master_password()));
        end if;
        return new;
    end;
    $code$;
    execute format('alter function credentials_update set search_path to %I,public', schema);

    create trigger credentials_update
        instead of update or insert
        on credentials
        for each row
    execute function credentials_update();

end;
$$;