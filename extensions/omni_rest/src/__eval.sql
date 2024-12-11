create function __eval(code text)
    returns text
    language plpgsql as
$$
declare
    result text;
begin
    execute 'select ' || code into result;
    return result;
end;
$$;
