create or replace function get_test_data() returns setof test
language plpgsql
as $$
begin
    return query select * from test;
end;
$$;
