create function default_page(request http_request) returns setof http_outcome as
$$
begin
    return query (with
                      stats as (select * from pg_catalog.pg_stat_database where datname = current_database())
                  select *
                  from
                      omni_httpd.http_response(headers => array [omni_http.http_header('content-type', 'text/html')],
                                               body => $html$
       <!DOCTYPE html>
       <html>
         <head>
           <title>Omnigres</title>
           <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bulma@0.9.4/css/bulma.min.css">
           <meta name="viewport" content="width=device-width, initial-scale=1">
         </head>
         <body class="container">
         <section class="section">
           <div class="container">
             <h1 class="title">Omnigres</h1>

             <div class="tile is-ancestor">
                <div class="tile is-parent is-8">
                 <article class="tile is-child notification is-primary">
                   <div class="content">
                     <p class="title">Welcome!</p>
                     <p class="subtitle">What's next?</p>
                     <div class="content">
                     <p>You can update the query in the <code>omni_httpd.handlers</code> table to change this default page.</p>

                     <p><a href="https://docs.omnigres.org">Documentation</a></p>
                     </div>
                   </div>
                 </article>
               </div>
               <div class="tile is-vertical">
                 <div class="tile">
                   <div class="tile is-parent is-vertical">
                     <article class="tile is-child notification is-grey-lighter">
                       <p class="title">Database</p>
                       <p class="subtitle"><strong>$html$ || current_database() || $html$</strong></p>
                       <p> <strong>Backends</strong>: $html$ || (select numbackends from stats) || $html$ </p>
                       <p> <strong>Transactions committed</strong>: $html$ || (select xact_commit from stats) || $html$ </p>
                     </article>
                   </div>
                 </div>
               </div>
             </div>

             <p class="is-size-7">
               Running on <strong> $html$ || version() || $html$ </strong>
             </p>
           </div>
         </section>
         </body>
       </html>
       $html$));
end;
$$ language plpgsql;

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