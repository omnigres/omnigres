create function always_true() returns boolean
language plpgsql
as $$
begin
    return true;
end;
$$;
