# Using CloudEvents

This extension lets you create, validate, and publish standardized [CloudEvents](https://cloudevents.io/) events directly from your SQL workflows.

!!! tip "`omni_cloudevents` is a templated extension"

    `omni_cloudevents` is a templated extension. This means that by installing it, a default copy of it is instantiated into extension's schema. However, you can replicate it into any other schema, tune some of the parameters and make the database own the objects (as opposed to the extension itself):

     ```postgresql 
     select omni_cloudevents.instantiate([schema => 'omni_cloudevents'])
     ```

    This allows you to have multiple independent credential systems, even if
    using different versions of `omni_cloudevents`. 

## Getting Started 

!!! tip "Add cloudevents schema to `search_path`"

    In order to make your queries that work with omni_cloudevents easier to read,
    consider adding `omni_cloudevents` (or the schema of your choosing if you instantiated the
    template) to `search_path`:

    ```postgresql
    set search_path to omni_cloudevents, public;
    ```

    Examples in this documentation follow this suggestion.


## Preparing events

In order to prepare events, use the `cloudevent` function. It features a number of mandatory and
optional arguments to let you form the event that you need.

```postgresql
select cloudevent(
  id => gen_random_uuid(),
  source => 'https://service.com/endpoint',
  type => 'user.login'
);
```

|        **Argument** | **Type**               | **Description**                                                                                                        |
|--------------------:|------------------------|------------------------------------------------------------------------------------------------------------------------|
|              **id** | `text` _or_ `uuid`     | Unique event identifier (text string or UUID value)                                                                    |
|          **source** | `cloudevent_uri_ref`   | Event origin URI with URI reference validation ([RFC 3986](https://datatracker.ietf.org/doc/html/rfc3986#section-4.1)) |
|            **type** | `text`                 | Event type descriptor (e.g., "app.order.processed")                                                                    |
| **datacontenttype** | `text`                 | _(Optional)_ Content type of data payload (e.g., "application/json")                                                   |
|       **datschema** | `cloudevent_uri`       | _(Optional)_ Schema URL for data payload validation                                                                    |
|         **subject** | `text`                 | _(Optional)_ Event subject/context identifier                                                                          |
|              **ts** | `timestamptz`          | _(Optional)_ Event timestamp (default: current statement time)                                                         |
|            **data** | `anyelement`           | _(Optional)_ Payload content (supports any PostgreSQL data type)                                                       |
|     **specversion** | `text`                 | _(Optional)_ CloudEvents specification version (default: '1.0')                                                        |

## Publishing events

In order to publish an event we can this helper:

```postgresql
select publish(
  cloudevent(id => gen_random_uuid(), 
            source => 'https://api.yourservice.com/sys', 
            type => 'file.uploaded', 
             data => 'data-lake-bucket-123'::text));
```

It will return the `id` of the event for further convenience:

```
               publish                
--------------------------------------
 8d253e18-c49a-464d-abcd-c2f7f84e3c46
(1 row)
```

Under the hood, it'll write it into the `cloudevent_egress` table. This helps us enforce outgoing
message uniqueness and manage the audit trail.

```postgresql
select * from cloudevents_egress;
```

```
-[ RECORD 1 ]---+-------------------------------------------
id              | 8d253e18-c49a-464d-abcd-c2f7f84e3c46
source          | https://api.yourservice.com/sys
specversion     | 1.0
type            | file.uploaded
datacontenttype | 
datschema       | 
subject         | 
time            | 2025-01-31 11:54:06.249177-08
data            | \x646174612d6c616b652d6275636b65742d313233
datatype        | text
```

## Published event consumption

Event systems hugely benefits from reactivity – the sooner the event reaches the
intended recipients, the sooner they can take necessary action. In order to facilitate this,
`omni_cloudevents` has a system of "publishers" that are triggered on event insertion 

!!! warning "Work in progress"

    Currently, there is a very limited set of publishers (namely, `NOTICE` publisher) but this
    is planned to be extended in the near future.

### NOTICE publisher

To create a NOTICE publisher to be used by a `psql` session or tooling that makes use
of such notifications, you can call this idempotent function below. It will return the
singleton name of this publisher.

```postgresql
select omni_cloudevents.create_notice_publisher();
```

Now, if you will publish an event, you will see a notification:

```postgresql
select publish(
  cloudevent(id => gen_random_uuid(), 
            source => 'https://api.yourservice.com/sys', 
            type => 'file.uploaded', 
             data => 'data-lake-bucket-123'::text));
```

```
NOTICE:  {"id":"1c9e6c07-ddcc-4de1-987b-e26ec9e8d253","source":"https://api.yourservice.com/sys","specversion":"1.0","type":"file.uploaded","time":"2025-01-31T12:00:34.147227-08:00","data":"data-lake-bucket-123"}
```

To delete the publisher, simply call `delete_publisher` with the name you received from `create_notice_publisher`:

```postgresql
select omni_cloudevents.delete_publisher(name);
```

!!! tip "Observing events in flight"

    If your tooling needs to see events that weren't committed yet (for example, to provide a more responsive
    experience within a transaction), notice publisher can be made to observe uncommitted events as well:
    
    ```postgresql
    select omni_cloud_events.create_notice_publisher(publish_uncommitted => true);
    ```