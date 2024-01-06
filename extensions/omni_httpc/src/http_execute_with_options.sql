create function http_execute_with_options(options http_execute_options, variadic requests http_request[]) returns setof http_response
as
'MODULE_PATHNAME',
'http_execute' language c;