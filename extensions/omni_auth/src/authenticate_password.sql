create function authenticate(password password, auth_subject_id authentication_subject_id,
                             as_of timestamptz default statement_timestamp())
    returns password_authentication_attempts
    language plpgsql
as
$$
declare
    result omni_auth.password_authentication_attempts;
begin
    with credentials as (select hashed_password
                         from omni_auth.password_credentials
                         where valid_at @> as_of
                           and authentication_subject_id = auth_subject_id),
         validation as (select auth_subject_id as subject_id,
                               hashed_password,
                               case
                                   when omni_auth.hashing_algorithm(hashed_password) = 'bcrypt' then
                                       crypt(password, hashed_password)
                                   -- unknown algorithm, pretend we're still doing hashing
                                   else omni_auth.hash_password(password)
                                   end as hashed_attempted_password
                        from credentials
                        -- otherwise, pretend to do work
                        union
                        select null as subject_id, '', omni_auth.hash_password(password)
                        where not exists (select from credentials))
    insert
    into omni_auth.password_authentication_attempts (authentication_subject_id, hashed_password, success)
    select subject_id,
           hashed_attempted_password,
           hashed_attempted_password = hashed_password
    from validation
    returning * into result;
    return result;
end
$$;