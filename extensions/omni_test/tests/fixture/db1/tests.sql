create procedure test1(inout test omni_test.test)
    language plpgsql
as
$$
begin
    create table test
    (
    );
end;
$$;

comment on procedure test1 is $$Test 1$$;

create procedure test2(inout test omni_test.test)
    language plpgsql
as
$$
begin
    -- Intentionally conflicting with test1
    create table test
    (
    );
end;
$$;

comment on procedure test2 is $$Test 2$$;


create procedure err(inout test omni_test.test)
    language plpgsql
as
$$
begin
    raise exception 'failed test';
end;
$$;

comment on procedure err is $$Error test$$;

create function test_fun() returns omni_test.test
    language plpgsql as
$$
begin
    return null;
end;
$$;

comment on function test_fun is $$Test function$$;