create procedure spi_exec(stmts text, atomic boolean default true)
    language c as
'MODULE_PATHNAME';