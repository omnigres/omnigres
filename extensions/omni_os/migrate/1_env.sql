create function environment_variables()
    returns table (variable text, value text)
as
'MODULE_PATHNAME',
'environment_variables'
    language c;

create view env as (select * from environment_variables());
