create or replace function cookie_get(cookies text, cookie_name text)
    returns text
    strict
    immutable
    language plpgsql
as
$$
declare
    cookie text;
begin
    foreach cookie in array string_to_array
                            (cookies, '; '
                            )
        loop
            if split_part(cookie, '=', 1) = cookie_name then
                return split_part(cookie, '=', 2);
            end if;
        end loop;
    return null;
end;
$$;