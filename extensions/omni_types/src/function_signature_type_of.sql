create function function_signature_type_of(name name, func text)
    returns regtype
    language plpgsql
as
$$
declare
    result regtype;
    proc   regprocedure;
begin
    set local search_path to omni_polyfill, '$user', public, pg_catalog;
    begin
        proc = func::regprocedure;
    exception
        when others then
            proc = func::regproc;
    end;
    select omni_types.function_signature_type(name, variadic
                                              trim_array(
                                                      array_append(pg_proc.proargtypes::oid[], pg_proc.prorettype)::regtype[],
                                                      0))
    from pg_proc
    where oid = proc
    into result;
    return result;
end;
$$;