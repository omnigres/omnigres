# Quick start

omni_proxy allows you to proxy databases in the same cluster using a single entry point.
This can be useful when developing modular applications that should have the same entry point.

In the example below we have 2 databases, each with their own omni_httpd listener and we want to access both through a third database used just as a gateway.

For this example, let's assume we have one application in a database called `accounting`, another one called `store`. We want to access endpoints on both using a path prefix such that the path `/accounting/` will respond as `/` from the accounting application.

## Creating target databases

Just for the sake of testing we can create two target databases containing only the default omni_httpd route.

On a psql console connected to your Omnigres cluster:

```postgresql
create database store;
\c store
create extension omni_httpd cascade;
```

To view what port is being used one can query the `omni_httpd.listeners` relation:

```postgresql 
select effective_port from omni_httpd.listeners;
```

And now testing this using curl the default route should be returned.
Try the curl command on your shell (remember that the port number 8080 is the default but you might need to replace it):

```sh 
curl http://localhost:8080/
```

There should be an HTML page with the name of the database responding the request.
Something similar to

```html 
         ...
         <p class="title">Database</p>
         <p class="subtitle"><strong>store</strong></p>
         ...
```

Now we can repeat the creation procedure for our second target database:

```postgresql
create database accounting;
\c accounting
create extension omni_httpd cascade;
```

## Setting up the gateway

Assuming both target databases were created successfully and are running omni_httpd workers, let's create a database called gateway and use that to proxy requests.

```postgresql
create database gateway;
\c gateway
create extension omni_proxy cascade;
select omni_proxy.proxy_listener((select id from omni_httpd.listeners))
```

Now inspect `omni_httpd.listeners` to find out the proxy port:

```postgresql 
gateway=# select effective_port from omni_httpd.listeners;
 effective_port 
----------------
    <proxy-port>
(1 row)
```

Now you can test the proxy using curl. Remember to replace <proxy-port> with the actual port number.

```sh
curl http://localhost:<proxy-port>/store/
```

And to access the accounting root path:

```sh
curl http://localhost:<proxy-port>/accounting/
```

You should be able to verify the name of the responding database on the page served by the default handler.

For more information on omni_httpd routing and handlers check [its documentation](../../omni_httpd/intro/).
