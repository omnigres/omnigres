-- Initialization
with
    config as
        (select
             coalesce(not current_setting('omni_httpd.no_init', true)::bool, true)     as should_init,
             coalesce(current_setting('omni_httpd.init_listen_address', true),
                      '0.0.0.0')::inet                                                 as init_listen_address,
             coalesce(current_setting('omni_httpd.init_port', true)::port, 8080::port) as init_port),
    listener
        as (insert into listeners (address, port) select init_listen_address, init_port from config where should_init returning id),
    handler as (insert into handlers (query) (select
                                                  query
                                              from
                                                  (values
                                                       ($$
       SELECT omni_httpd.default_page(request.*::omni_httpd.http_request) FROM request $$)) v(query),
                                                                                            config
                                              where
                                                  should_init) returning id)
insert
into
    listeners_handlers (listener_id, handler_id)
select
    listener.id,
    handler.id
from
    config,
    listener,
    handler
where
    config.should_init;