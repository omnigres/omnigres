create function hashing_algorithm(hashed_password hashed_password) returns hashing_algorithm
    language sql
    immutable parallel safe as
$$
select case
           when hashed_password ~ '^\$2[aby]\$' then
               'bcrypt'
           else null::omni_auth.hashing_algorithm
           end
$$;