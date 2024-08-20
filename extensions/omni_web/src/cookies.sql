create function cookies(cookies text)
    returns table
            (
                name  text,
                value text
            )
    immutable
    language plpgsql
as
$$
declare
    cookie text;
begin
    foreach cookie in array string_to_array(cookies, '; ')
        loop
            name := split_part(cookie, '=', 1);
            value := split_part(cookie, '=', 2);
            return next;
        end loop;
    return;
end;
$$;