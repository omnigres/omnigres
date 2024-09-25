create function hash_password(password password, hashed_password hashed_password default null,
                              hashing_algorithm hashing_algorithm default 'bcrypt',
                              work_factor integer default null)
    returns hashed_password
    language plpgsql
    parallel safe
as
$$
declare
    _hashed_password omni_auth.hashed_password;
begin
    if hashed_password is not null then
        hashing_algorithm := omni_auth.hashing_algorithm(hashed_password);
        work_factor := omni_auth.work_factor(hashed_password);
    end if;
    if hashing_algorithm = 'bcrypt' then
        work_factor :=
                coalesce(work_factor, coalesce(current_setting('omni_auth.bcrypt_work_factor', true), 12::text)::int);
        _hashed_password := crypt(password, coalesce(hashed_password, gen_salt('bf'::text, work_factor)));
        if hashed_password is not null and _hashed_password != hashed_password then
            return null;
        end if;
        return _hashed_password;
    else
        raise exception 'Unknown or missing hashing_algorithm';
    end if;
end;
$$;