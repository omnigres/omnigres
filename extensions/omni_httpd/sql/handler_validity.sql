-- Invalid queries
insert
into
    omni_httpd.handlers (query)
values
    ($$SELECT * FROM no_such_table$$);
insert
into
    omni_httpd.handlers (query)
values
    ($$SELECT request.pth FROM request$$);
insert
into
    omni_httpd.handlers (query)
values
    ($$$$);
insert
into
    omni_httpd.handlers (query)
values
    ($$SELECT; SELECT$$);

-- Valid query at the end of the transaction
begin;
insert
into
    omni_httpd.handlers (query)
values
    ($$SELECT * FROM no_such_table$$);
create table no_such_table
(
);
end;

drop table no_such_table;
