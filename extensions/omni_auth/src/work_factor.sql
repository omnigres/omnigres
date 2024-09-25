create function work_factor(hashed_password hashed_password) returns int
    language sql
    immutable parallel safe as
$$
select case
           when omni_auth.hashing_algorithm(hashed_password) = 'bcrypt' then
               split_part(hashed_password, '$', 3)::int
           else null
           end
$$;