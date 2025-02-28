`omni_httpd` dispatching can include a static file server to handle serving 
static assets through `omni_vfs` virtual filesystem later.

## Requirements

The following extensions are required for this functionality:

* omni_vfs
* omni_mimetypes

## Setup

To get the handler provisioned, one need to call `instantiate_static_file_handler`:

```postgresql
select omni_httpd.instantiate_static_file_handler(schema => 'public');
```

This will create the `static_file_handler` function in the `public` schema. Its name can be configured
by passing `name` argument with a desired function name.

## Configuring 

First, we'll define a static file router relation for the type of the virtual file
system we need (in this example, `omni_vfs.local_fs`):

```postgresql
create table static_file_router
(
    like omni_httpd.urlpattern_router,
    fs omni_vfs.local_fs
);
```

Now, we can implement the handler for the router:

```postgresql
create function fs_handler(req omni_httpd.http_request, router static_file_router)
    returns omni_httpd.http_outcome
    return static_file_handler(req, router.fs);
```

This function will take the request path as is, and it will try to find it in the given file system. 

!!! tip "Directory listing"

    `static_file_handler` also takes an optional boolean `listing` argument that will make it
    generate a list of files in a directory if there's no `index.html` present. It's disabled by default.

Finally, we need to provision a routing entry in the router:

```postgresql
insert
into
    static_file_router (match, handler, fs)
values
    (omni_httpd.urlpattern('/assets/*'), 'docs_handler'::regproc,
     omni_vfs.local_fs('/path/to/files'));
```
Now, all requests to `/assets/*` will be served the local filesystem pointing to `/path/to/files`.