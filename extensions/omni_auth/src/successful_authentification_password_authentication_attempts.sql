create function successful_authentication(password_authentication_attempts)
    returns boolean
    language sql
    immutable parallel safe
as
$$
select ($1).success
$$;