create function deny_operation_on_immutable_table() returns trigger as
$$
begin
    raise exception 'records in this table are immutable';
end;
$$ language plpgsql;