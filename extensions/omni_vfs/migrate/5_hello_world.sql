create function hello_world() returns void as
$$
begin
    raise info 'hello world';
end;
$$ language plpgsql;