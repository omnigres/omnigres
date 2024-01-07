create function http_execute(variadic requests http_request[]) returns setof http_response
as
$$
select
    omni_httpc.http_execute_with_options(omni_httpc.http_execute_options(), variadic requests);
$$ language sql;