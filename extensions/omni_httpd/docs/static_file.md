`omni_httpd` dispatching can include a static file server to handle serving 
static assets through `omni_vfs` virtual filesystem later.

## Requirements

The following extensions are required:

* omni_vfs
* omni_mimetypes


## Setup

As discussed in `omni_vfs` documentation, one needs to define a mount point. 

```postgresql
create function mount_point() 
    returns omni_vfs.local_fs
    language sql as
$$
select omni_vfs.local_fs('/path/to/dir')
$$
```

This example above returns a local filesystem-based VFS. In production, you 
may want to consider other filesystems.

Now, we want to update our listener handler to serve this filesystem 
alongside with some other endpoints:

```postgresql
 update omni_httpd.handlers
 set
     query =
(select
    omni_httpd.cascading_query(name, query order by priority desc nulls last)
 from (select * from omni_httpd.static_file_handlers('mount_point', 0)
      union (values
                 ('test',
                  $$ select omni_httpd.http_response('passed') from request where request.path = '/test'$$, 1))) routes)
```

The above connects `mount_point()` filesystem and defines a handler for 
`/test` (with a higher priority).