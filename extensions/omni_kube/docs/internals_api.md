# Internals: Kubernetes API

## `omni_kube.api()`

This is the central function to invoke Kubernetes API calls. It supports both single and batch requests.

### Single Request

```sql
omni_kube.api(path, [server], [cacert], [clientcert], [token], [method], [body], [stream])
```

|  **Parameter** | **Type**                      | **Description**                                                 |
|---------------:|-------------------------------|-----------------------------------------------------------------|
|       **path** | text                          | Request path (must start with `/`)                              |
|     **server** | text                          | Kubernetes server, defaults to `https://kubernetes.default.svc` |
|     **cacert** | text                          | CA certificate                                                  |
| **clientcert** | omni_httpc.client_certificate | Client certificate                                              |
|      **token** | text                          | Bearer token                                                    |
|     **method** | omni_http.http_method         | HTTP method, defaults to `GET`                                  |
|       **body** | jsonb                         | Request body                                                    |
|     **stream** | boolean                       | Stream mode for multiple JSON objects, defaults to `false`      |

**Returns:** `jsonb` - The response body

### Batch Request

```sql
omni_kube.api(paths, [server], [cacert], [clientcert], [token], [methods], [bodies], [stream])
```

|  **Parameter** | **Type**                      | **Description**                                                 |
|---------------:|-------------------------------|-----------------------------------------------------------------|
|      **paths** | text[]                        | Array of request paths                                          |
|     **server** | text                          | Kubernetes server, defaults to `https://kubernetes.default.svc` |
|     **cacert** | text                          | CA certificate                                                  |
| **clientcert** | omni_httpc.client_certificate | Client certificate                                              |
|      **token** | text                          | Bearer token                                                    |
|    **methods** | omni_http.http_method[]       | Array of HTTP methods (defaults to `GET` for all requests)      |
|     **bodies** | jsonb[]                       | Array of request bodies                                         |
|     **stream** | boolean                       | Stream mode for multiple JSON objects, defaults to `false`      |

**Returns:** `TABLE(response jsonb, status int2)` - Response body and HTTP status for each request

### Caching & Error Handling

- Single requests are cached per statement using request digest
- Single requests with status codes ≥ 400 raise exceptions with Kubernetes error details
- Stream mode converts newline-delimited JSON responses into JSONB arrays

## `omni_kube.watch()`

This function enables watching Kubernetes resources for changes using the Kubernetes watch API.

### Single Resource Watch

```sql
omni_kube.watch(group_version, resource, [resource_version], [timeout])
```

|        **Parameter** | **Type** | **Description**                                    |
|---------------------:|----------|----------------------------------------------------|
|    **group_version** | text     | API group version (e.g., `v1`, `apps/v1`)          |
|         **resource** | text     | Resource type (e.g., `pods`, `deployments`)        |
| **resource_version** | text     | Specific resource version to watch from (optional) |
|          **timeout** | int      | Watch timeout in seconds, defaults to `1`          |

**Returns:** `TABLE(events jsonb[], status int2)` - Events and HTTP status for each watch stream.

### Batch Resource Watch

```sql
omni_kube.watch(group_versions, resources, [resource_versions], [timeout])
```

|         **Parameter** | **Type** | **Description**                                     |
|----------------------:|----------|-----------------------------------------------------|
|    **group_versions** | text[]   | Array of API group versions                         |
|         **resources** | text[]   | Array of resource types                             |
| **resource_versions** | text[]   | Array of resource versions to watch from (optional) |
|           **timeout** | int      | Watch timeout in seconds, defaults to `1`           |

**Returns:** `TABLE(events jsonb[], status int2)` - Events and HTTP status for each watch stream.

### Behavior Notes

- **Resource Version Handling**: When `resource_version` is not specified, the function automatically fetches the
  current resource version and starts watching from that point
- **Timeout Behavior**: The function will wait for the specified timeout duration before returning. Setting a long
  timeout will cause the function to block for that entire duration

### Example Usage

```sql
-- Watch pods in the default namespace with 30-second timeout
select omni_kube.watch('v1', 'pods', timeout => 30);

-- Watch multiple resources simultaneously
select *
from omni_kube.watch(
        array ['v1', 'apps/v1'],
        array ['pods', 'deployments'],
        timeout => 60
     );
```

## Authentication & Certificates

**token** and **cacert** are automatically inferred from default pod's
paths (`/var/run/secrets/kubernetes.io/serviceaccount/token` and `/var/run/secrets/kubernetes.io/serviceaccount/ca.crt`
respectively) to enable seamless use of API from within pods (through `omni_kube.pod_credentials()` function). They can
be overridden by corresponding function parameters or `omni_kube.token` and `omni_kube.cacert` settings. In addition,
`omni_kube.clientcert` and `omni_kube.client_private_key` settings can be used to override
the `clientcert` parameter.
