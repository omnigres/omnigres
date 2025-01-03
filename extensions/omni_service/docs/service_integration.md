# Service Integration

!!! tip "This article is for other extension developers"

    If you are developing an extension that runs a service (similar to how
    [omni_httpd](/omni_httpd/intro) is, this section describes the basics of
    service integration.

## Service Registration

Services are recorded in the `services` table and can be registered using
`register_service(name [, info])` function. If a service with such a name already exists,
only `info` will be updated.

The services are recorded in the `services` table:

|                  **Name** | **Type**    | **Description**                                                                                  |
|--------------------------:|-------------|--------------------------------------------------------------------------------------------------|
|                  **name** | text        | The unique name of the service                                                                   |
|                  **info** | jsonb       | Additional metadata or configuration details about the service                                   |
| **postmaster_start_time** | timestamptz | The timestamp when the database server started (used with `name` as the primary key in services) |

`postmaster_start_time` allows us to distinguish services registered after postmaster has
been restarted. It is populated automatically.

This is typically not necessary as `services` are cleaned up after every registration to
only contain the current "epoch".

However, if relevant trigger (`service_provisioning_trigger`) has been disabled, the records are
kept and this allows to discern registrations.

## Operations

In order to integrate your service into `omni_service`, you need to add triggers to `service_operations` to observe the
operations and enact appropriate actions.

```postgresql
create function service_operation() returns trigger
    language plpgsql
as
$$
begin
    case
        when new.operation = 'stop' then call my_service.stop();
        when new.operation = 'start' then call my_service.start();
        else null;
        end case;
    return new;
end;
$$;

create or replace trigger my_service_operations
    after insert
    on service_operations
    for each row
execute function service_operation();
```