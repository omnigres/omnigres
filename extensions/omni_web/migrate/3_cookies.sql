create function cookies(cookies text)
    returns table
            (
                name  text,
                value text
            )
    language sql
as
$$
select
    split_part(cookie, '=', 1),
    split_part(cookie, '=', 2)
from
    unnest(string_to_array(cookies, '; ')) cookies_arr(cookie);
$$;