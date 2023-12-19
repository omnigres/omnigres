create function set_variable(name name, value anyelement) returns anyelement
    language plpgsql
    security definer -- to be able to access omni_var
as
$$
begin
    raise warning 'omni_txn.set_variable is deprecated, use omni_var.set';
    return omni_var.set(name, value);
end
$$;

create function get_variable(name name, default_value anyelement) returns anyelement
    language plpgsql
    security definer -- to be able to access omni_var
    stable as
$$
begin
    raise warning 'omni_txn.get_variable is deprecated, use omni_var.get';
    return omni_var.get(name, default_value);
end
$$;