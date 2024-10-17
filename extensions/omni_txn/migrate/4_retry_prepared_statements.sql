create function retry_prepared_statements()
    returns table
            (
                stmt text
            )
    language c
as
'MODULE_PATHNAME';

create view retry_prepared_statements as
select *
from retry_prepared_statements();

create function reset_retry_prepared_statements() returns void
    language c as
'MODULE_PATHNAME';