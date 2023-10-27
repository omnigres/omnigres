Below is a simple web application that runs inside of Postgres
and manages [MOTD (Message Of The Day)](https://en.wikipedia.org/wiki/Message_of_the_day).

All you need to run this is just an instance of Postgres with
Omnigres extensions (omni_httpd and omni_web) [installed](../quick_start.md).

```postgresql
{% include "./motd.sql" %}
```

1. We'll store MOTD here
2. Handles GET request
3. Handles GET request when there is no MOTD
4. Handles POST request
5. Here we update an existing listener's handler. This listener is provisioned
   by omni_httpd by default.
6. Cascading queries allow combining multiple handlers into one

It works like this:

```shell
GET / # => HTTP/1.1 200 OK
No MOTD

POST / "Check out Omnigres" # => HTTP/1.1 201 OK

GET / # => HTTP/1.1 200 OK
Posted at 2023-03-23 02:59:14.679113
Check out Omnigres
```

