create function set_password(authentication_subject_id authentication_subject_id, password password,
                             old_password password default null,
                             valid_from timestamptz default statement_timestamp(),
                             valid_until timestamptz default null,
                             hashing_algorithm hashing_algorithm default 'bcrypt',
                             work_factor integer default null)
    returns password_credentials
    language plpgsql
as
$$
declare
    result           omni_auth.password_credentials;
    _hashed_password omni_auth.hashed_password;
begin
    _hashed_password :=
            omni_auth.hash_password(password, hashing_algorithm => hashing_algorithm, work_factor => work_factor);
    with existing_credential as (select *
                                 from omni_auth.password_credentials
                                 where password_credentials.authentication_subject_id =
                                       set_password.authentication_subject_id
                                   and valid_at @> statement_timestamp()),
         credential as (select omni_auth.password_credential_id_nextval(),
                               existing_credential.authentication_subject_id,
                               case
                                   when old_password is null then _hashed_password
                                   when old_password is not null and omni_auth.hash_password(password => old_password,
                                                                                             hashed_password => hashed_password) is not null
                                       then
                                       _hashed_password
                                   else
                                       null
                                   end,
                               tstzrange(valid_from, coalesce(valid_until, upper(valid_at)))
                        from existing_credential
                        union
                        select omni_auth.password_credential_id_nextval(),
                               authentication_subject_id,
                               _hashed_password,
                               tstzrange(valid_from, coalesce(valid_until, 'infinity'))
                        where not exists (select from existing_credential)),
         update_validity
             as (update omni_auth.password_credentials set valid_at = tstzrange(lower(password_credentials.valid_at),
                                                                                valid_from) from existing_credential where password_credentials.id = existing_credential.id returning password_credentials.*),
         update as (select *
                    from update_validity
                    union
                    select (null::omni_auth.password_credentials).*
                    where not exists (select from update_validity))
    insert
    into omni_auth.password_credentials
    select credential.*
    from credential,
         update
    returning password_credentials.* into result;
    return result;
exception
    when not_null_violation then
        raise exception 'incorrect old_password';
end;
$$;