# omni_mimetypes

This extension contains MIME types and their attributes collected from various sources:

* [IANA](https://www.iana.org/assignments/media-types/media-types.xhtml)
* [Apache](https://svn.apache.org/repos/asf/httpd/httpd/trunk/docs/conf/mime.types)
* [Nginx](https://hg.nginx.org/nginx/raw-file/default/conf/mime.types)

The extension is implemented purely in SQL and PL/pgSQL.

!!! tip "Credits"

    The data has been fetched from [mime-db](https://github.com/jshttp/mime-db), authors and maintainers of which did all the hard work.

## Examples

### Querying MIME type for file extension

```postgresql
select
    mime_types.name
from
    omni_mimetypes.mime_types
    inner join omni_mimetypes.mime_types_file_extensions mtfe on mtfe.mime_type_id = mime_types.id
    inner join omni_mimetypes.file_extensions on mtfe.file_extension_id = file_extensions.id
where
    file_extensions.extension = 'js'
```

Result:

```
          name          
------------------------
 application/javascript
(1 row)
```

## Tables

### mime_types

Contains all the MIME types obtained from the upstream databases or custom-added.

|           Column | Type             | Description                                                        |
|-----------------:|------------------|--------------------------------------------------------------------|
|           **id** | integer          | Primary key                                                        |
|         **name** | text             | Unique name                                                        |
|       **source** | mime_type_source | `iana`, `apache`, `nginx` or `NULL` (probably a custom media type) |
| **compressible** | bool             | Whether a file of this type can be gzipped (nullable)              |
|      **charset** | text             | The default charset associated with this type, if any.             |


### file_extensions

Contains all known file extensions.

|        Column | Type    | Description                              |
|--------------:|---------|------------------------------------------|
|        **id** | integer | Primary key                              |
| **extension** | text    | File extension (without a preceding dot) |

### mime_types_file_extensions

Maps file extensions to MIME types.

|                Column | Type    | Description                           |
|----------------------:|---------|---------------------------------------|
|      **mime_type_id** | integer | Foreign key or **mime_types.id**      |
| **file_extension_id** | integer | Foreign key or **file_extensions.id** |

## Updating the database

One can manually modify the above tables or fetch a newer version of `mime-db` using
`import_mime_db` function:

```postgresql
create extension omni_httpc cascade; -- (1)
with
    db as (select *
           from
               omni_httpc.http_execute(
                   omni_httpc.http_request(
                       'https://cdn.jsdelivr.net/gh/jshttp/mime-db@1.52.0/db.json')) as import) -- (2)
select
    omni_mimetypes.import_mime_db(convert_from(body, 'utf8')::jsonb)
from
    db;
```

1. This gives us an HTTP client.
2. Adjust `mime-db` version as desired