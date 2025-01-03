# Service Management

This extension provides a uniform way of managing services like [omni_httpd](/omni_httpd/intro)
in a general way. For example, you want to stop all running services? You don't need to care
how these particular services implement such functionality. You simply do something like:

```postgresql
select omni_service.service_stop(name)
from omni_service.current_services
where state = 'running';
```

!!! tip "`omni_service` is a templated extension"

    `omni_service` is a templated extension. This means that by installing it, none of the intended objects are provisioned, but that can be done by instantiating a template:

     ```postgresql 
     select omni_service.instantiate([schema => 'omni_service'])
     ```

## Service Listing

`current_services` table shows the list of services along with their current status.

|  **Name** | Type          | Description                                                                                  |
|----------:|---------------|----------------------------------------------------------------------------------------------|
|  **name** | text          | The unique identifier for the service                                                        |
|  **info** | jsonb         | Additional metadata or configuration details for the service                                 |
| **state** | service_state | The current operational state of the service (one of `not-running`, `running`, or `stopped`) |

## Starting and Stopping a Service

`service_start(name)` and `omni_service.service_stop(name)` will respectively start and stop
a service if it is in the right state.

Every such operation is logged in the `service_operations` table:

|                  **Name** | **Type**          | **Description**                                                      |
|--------------------------:|-------------------|----------------------------------------------------------------------|
|                  **name** | text              | The unique identifier of the service to which this operation applies |
| **postmaster_start_time** | timestamptz       | The database server start time, referencing the `services` table     |
|             **operation** | service_operation | The type of operation performed (e.g., `start` or `stop`)            |
|            **applied_at** | timestamptz       | The timestamp when the operation was recorded                        |


