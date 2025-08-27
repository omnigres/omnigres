# Composable Paths

omni_kube provides a set of path helper functions to construct valid Kubernetes API URLs. These functions
handle the complexity of Kubernetes API path conventions and query parameter encoding. They can be then used
in [`api()`](internals_api.md#omni_kubeapi) calls.

## Core Path Functions

### `group_path(group_version text)`

Constructs the base API path for a given API group version.

**Parameters:**

- `group_version` (text): The Kubernetes API group version

**Returns:** Base API path as text

**Examples:**

```postgresql
select group_path('v1');
-- Returns: /api/v1

select group_path('apps/v1');
-- Returns: /apis/apps/v1

select group_path('networking.k8s.io/v1');
-- Returns: /apis/networking.k8s.io/v1
```

### `resources_path(group_version text, resource text, namespace text default null)`

Constructs the full resource path, including optional namespace.

**Parameters:**

- `group_version` (text): The Kubernetes API group version
- `resource` (text): The resource type (plural form)
- `namespace` (text, optional): The namespace for namespaced resources

**Returns:** Full resource path as text

**Examples:**

```postgresql
-- Cluster-scoped resources
select resources_path('v1', 'nodes');
-- Returns: /api/v1/nodes

-- Namespaced resources
select resources_path('v1', 'pods', 'default');
-- Returns: /api/v1/namespaces/default/pods

select resources_path('apps/v1', 'deployments', 'kube-system');
-- Returns: /apis/apps/v1/namespaces/kube-system/deployments
```

### `watch_path(path text, resource_version text default null)`

Converts a regular API path to a watch path for streaming resource changes.

**Parameters:**

- `path` (text): The base API path
- `resource_version` (text, optional): Starting resource version for the watch

**Returns:** Watch-enabled path with query parameters

**Examples:**

```postgresql
-- Basic watch path
select watch_path('/api/v1/pods');
-- Returns: /api/v1/pods?watch=1

-- Watch from specific resource version
select watch_path('/api/v1/pods', '12345');
-- Returns: /api/v1/pods?watch=1&resourceVersion=12345

-- Watch path with existing query parameters
select watch_path('/api/v1/pods?limit=100', '12345');
-- Returns: /api/v1/pods?limit=100&watch=1&resourceVersion=12345
```

###

`path_with(path text, label_selector text default null, field_selector text default null, timeout_seconds int default null)`

Adds common query parameters to API paths with proper URL encoding.

**Parameters:**

- `path` (text): The base API path
- `label_selector` (text, optional): Label selector for filtering
- `field_selector` (text, optional): Field selector for filtering
- `timeout_seconds` (int, optional): Request timeout in seconds

**Returns:** Path with encoded query parameters

**Examples:**

```postgresql
-- Add label selector
select path_with('/api/v1/pods', 'app=nginx');
-- Returns: /api/v1/pods?labelSelector=app%3Dnginx

-- Add multiple selectors
select path_with('/api/v1/pods', 'app=nginx', 'status.phase=Running');
-- Returns: /api/v1/pods?labelSelector=app%3Dnginx&fieldSelector=status.phase%3DRunning

-- Add timeout
select path_with('/api/v1/pods', timeout_seconds => 30);
-- Returns: /api/v1/pods?timeoutSeconds=30

-- Complex selector with encoding
select path_with('/api/v1/pods', 'environment in (prod,staging)', 'metadata.name!=test-pod');
-- Returns: /api/v1/pods?labelSelector=environment%20in%20%28prod%2Cstaging%29&fieldSelector=metadata.name%21%3Dtest-pod
```

## Resource Table Path Functions

For each [resource table](resources.md#resource-tables) created by omni_kube, a dedicated path function is automatically
generated with the naming pattern `{table_name}_resource_path()`.

### Usage Examples

```postgresql
-- Get the resource path for pods table
select pods_resource_path();
-- Returns: /api/v1/pods

-- Use in API calls
select omni_kube.get(pods_resource_path());
```

## URL Encoding

The path helper functions automatically handle URL encoding through `omni_web.url_encode()`:

- **Spaces** → `%20`
- **Equals** (`=`) → `%3D`
- **Commas** (`,`) → `%2C`
- **Parentheses** → `%28`, `%29`
- **Exclamation marks** (`!`) → `%21`

This ensures that complex label and field selectors work correctly with the Kubernetes API.

## Best Practices

1. **Use resource table path functions** when available for consistency
2. **Combine functions** to build complex paths step by step
3. **Let the functions handle encoding** - don't pre-encode parameters
4. **Test complex selectors** with simple API calls before using in production
5. **Cache frequently used paths** in variables or functions for performance

These path helpers abstract away the complexity of Kubernetes API URL construction, making it easier to work with
various resource types and query patterns.
