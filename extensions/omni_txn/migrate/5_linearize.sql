create function linearize() returns void
    language c
as
'MODULE_PATHNAME';

create function linearized() returns boolean
    language c
as
'MODULE_PATHNAME';

drop procedure retry(stmts text, max_attempts int, repeatable_read boolean,
                     collect_backoff_values boolean, params record);

create procedure retry(stmts text, max_attempts int default 10, repeatable_read boolean default false,
                       collect_backoff_values boolean default false,
                       params record default null::record, linearize boolean default false)
    language c as
'MODULE_PATHNAME';
