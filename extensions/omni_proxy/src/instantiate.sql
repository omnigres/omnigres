create or replace function instantiate(schema name default 'omni_proxy')
    returns void
    language plpgsql
as
$instantiate$
begin
    perform set_config('search_path', schema::text || ',public', true);
    /*{% include "proxy_handler.sql" %}*/
end;
$instantiate$;
