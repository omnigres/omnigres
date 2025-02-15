# Routing

`omni_httpd` implements routing using the concepts of routers and routes.

## Router

A router is a relation (table) that defines how incoming HTTP requests should be handled. It must include at least the following:

### Matcher Column

A column (with any name) of type `omni_httpd.urlpattern` that defines the URL pattern to match.

`omni_httpd.urlpattern` composite type is defined as below and has a matching `omni_httpd.urlpattern()`
constructor function that takes all of these parameters (with `null` values by default to signify acceptance of
"any value")

|     **Name** | Type                  | Description                                      |
|-------------:|-----------------------|--------------------------------------------------|
| **protocol** | text                  | The protocol part of the URL (e.g., http, https) |
| **username** | text                  | The username for authentication (if provided)    |
| **password** | text                  | The password for authentication (if provided)    |
| **hostname** | text                  | The hostname or domain of the URL                |
|     **port** | int                   | The port number specified in the URL             |
| **pathname** | text                  | The path segment of the URL                      |
|   **search** | text                  | The query string component of the URL            |
|     **hash** | text                  | The fragment identifier of the URL               |
|   **method** | omni_http.http_method | The HTTP method (e.g., GET, POST, etc.)          |

### Handler Column

A column (with any name) of type `regprocedure` that specifies the function to execute when the URL pattern is matched.

Values referring to functions and procedures here will be treated depending on their parameters. Procedures will be called
with a non-atomic context [^nonatomic]

[^nonatomic]: This means one can run transactional commands like `rollback` and `commit`

Following parameters may be specified, at any position:

* `omni_httpd.http_request` – will pass the request
* `omni_httpd.http_outcome` – for procedural handlers and middleware 
  * For handlers, this parameter must use the **out** mode.
  * For middleware, it should use **inout** mode and will not terminate the request processing.
* `<router table type>` – the matched record from the router can be passed into the handler.

Note: The function must return a value of type omni_httpd.httpd_outcome.

#### Examples

##### Request passing function handler

```postgresql
create function normal_handler(request omni_httpd.http_request)
  returns omni_httpd.http_outcome
  return omni_httpd.http_response(request.path);
```

This function will return a response with the request's path.

##### No arguments function handler

```postgresql
create function function_handler()
  returns omni_httpd.http_outcome
  return omni_httpd.http_response('function');
```

This function will simply return a response without considering the request.

##### Passing route tuple function

```postgresql
create function tuple_handler(t my_router)
  returns omni_httpd.http_outcome
  return omni_httpd.http_response(format('%s', t.handler))
```

This function will return the route's handler (itself: `tuple_handler`)

### Priority Column

An **optional** column (with any name) of type omni_httpd.route_priority that indicates the route’s priority. A higher number means a higher priority.

---

Every row in this relation denotes a route. Upon HTTP request, `omni_httpd` will match the incoming request against all
routes in all routers.

---

### Default router

`omni_httpd` provides a default router (`omni_httpd.urlpattern_router`) that can be used either directly
(by inserting records into it) or as a "template" to be used in creation of own routers to re-use the
boilerplate by using `like omni_httpd.urlpattern_router` construct.

## Example

```postgresql
create function my_handler(request omni_httpd.http_request)
  returns omni_httpd.http_outcome
  return omni_httpd.http_response(body => request.headers::text);

create table my_router (like omni_httpd.urlpattern_router);

insert into my_router (match, handler)
values (omni_httpd.urlpattern('/headers'), 'my_handler'::regproc);
```

We can now test it:

```shell
$ curl localhost:8080/headers
{"(omnigres-connecting-ip,127.0.0.1)","(user-agent,curl/8.7.1)","(accept,*/*)"}
```