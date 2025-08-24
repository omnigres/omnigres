# Credentials

omni_kube provides flexible credential loading capabilities that support multiple Kubernetes authentication methods,
including kubeconfig files and in-cluster service account credentials.

## Overview

The credential loading system supports:

1. **Kubeconfig-based authentication** - Load credentials from standard Kubernetes configuration files
2. **In-cluster authentication** - Automatic detection and use of service account credentials when running inside a
   Kubernetes pod
3. **Multiple authentication methods** - Support for certificates, tokens, and file-based credentials

## Functions

### `omni_kube.load_kubeconfig(config jsonb, context text default null, local boolean default false)`

Loads Kubernetes credentials from a parsed kubeconfig JSON object.

**Parameters:**

- `config` (jsonb): The parsed kubeconfig as a JSON object
- `context` (text, optional): The context to use. If null, uses the `current-context` from the config
- `local` (boolean, default false): If true, configuration is only visible to the current transaction. If false,
  configuration persists for the entire session

**Example:**

```postgresql
-- Load from a JSONB config object
select omni_kube.load_kubeconfig('{"current-context": "my-cluster", ...}'::jsonb);

-- Use a specific context
select omni_kube.load_kubeconfig(config_object, 'production-cluster');
```

### `omni_kube.load_kubeconfig(config json, context text default null, local boolean default false)`

Loads Kubernetes credentials from a JSON kubeconfig object (automatically converts to jsonb).

**Parameters:**

- `config` (json): The kubeconfig as a JSON object
- `context` (text, optional): The context to use
- `local` (boolean, default false): Configuration scope (transaction vs session)

### `omni_kube.load_kubeconfig(filename text, context text default null, local boolean default false)`

Loads Kubernetes credentials directly from a kubeconfig file.

**Parameters:**

- `filename` (text): Path to the kubeconfig file
- `context` (text, optional): The context to use
- `local` (boolean, default false): Configuration scope (transaction vs session)

!!! warning "Permissions required"

    **Note**: This function uses `pg_read_file()` internally, which requires superuser privileges or the

    `pg_read_server_files` role. If you don't have these privileges, load the file contents manually and use the JSON/JSONB version instead:

     ```postgresql
     -- Alternative for non-superusers (requires external file reading)
     \set kubeconfig `cat ~/.kube/config`
     select omni_kube.load_kubeconfig(omni_yaml.to_json(:'kubeconfig'));
     ```

**Example:**

```postgresql
-- Load default kubeconfig (requires superuser or pg_read_server_files role)
select omni_kube.load_kubeconfig('/home/user/.kube/config');

-- Load specific kubeconfig with custom context
select omni_kube.load_kubeconfig('/path/to/kubeconfig', 'staging');
```

### `omni_kube.pod_credentials()`

Automatically detects and returns service account credentials when running inside a Kubernetes pod.

**Returns:**

- `token` (text): The service account token
- `cacert` (text): The cluster CA certificate

> **Note**: This function uses `pg_read_file()` and `pg_stat_file()` internally to access service account files mounted
> by Kubernetes. It requires superuser privileges or the `pg_read_server_files` role. This function is typically used
> when
> Postgres is running inside a Kubernetes pod with a service account.

**Example:**

```postgresql
-- Check for in-cluster credentials (requires superuser or pg_read_server_files role)
select *
from omni_kube.pod_credentials();
```

## Supported Authentication Methods

### 1. Client Certificates (Embedded)

Certificates and keys embedded directly in the kubeconfig as base64-encoded data:

```yaml
users:
- name: my-user
  user:
    client-certificate-data: LS0tLS1CRUdJTi... # base64 encoded cert
    client-key-data: LS0tLS1CRUdJTi...         # base64 encoded key
```

**Configuration Variables Set:**

- `omni_kube.clientcert` - The client certificate (PEM format)
- `omni_kube.client_private_key` - The client private key (PEM format)

### 2. Client Certificates (File References)

Certificates and keys stored in separate files:

```yaml
users:
- name: my-user
  user:
    client-certificate: /path/to/client.crt
    client-key: /path/to/client.key
```

> **Note**: This method requires file system access via `pg_read_file()`, which needs superuser privileges or the
`pg_read_server_files` role.

**Configuration Variables Set:**

- `omni_kube.clientcert` - The client certificate (loaded from file)
- `omni_kube.client_private_key` - The client private key (loaded from file)

### 3. Bearer Token (Embedded)

Token embedded directly in the kubeconfig:

```yaml
users:
- name: my-user
  user:
    token: eyJhbGciOiJSUzI1NiIsImtpZCI6...
```

**Configuration Variables Set:**

- `omni_kube.token` - The bearer token

### 4. Bearer Token (File Reference)

Token stored in a separate file:

```yaml
users:
- name: my-user
  user:
    tokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
```

> **Note**: This method requires file system access via `pg_read_file()`, which needs superuser privileges or the
`pg_read_server_files` role.

**Configuration Variables Set:**

- `omni_kube.token` - The bearer token (loaded from file)

### 5. In-Cluster Service Account

When running inside a Kubernetes pod, service account credentials are automatically available:

- Token: `/var/run/secrets/kubernetes.io/serviceaccount/token`
- CA Certificate: `/var/run/secrets/kubernetes.io/serviceaccount/ca.crt`
- Namespace: `/var/run/secrets/kubernetes.io/serviceaccount/namespace`

> **Note**: Accessing these files requires superuser privileges or the `pg_read_server_files` role. This is typically
> configured when deploying Postgres inside Kubernetes.

## Privilege Requirements

Several functions require elevated privileges to access the file system:

### Required Roles/Privileges

- **Superuser**: Full access to all file operations
- **pg_read_server_files**: Granted role that allows reading server files

### Granting Privileges

```postgresql
-- Grant pg_read_server_files role to a user
grant pg_read_server_files to myuser;

-- Or create a superuser (use with caution)
alter user myuser superuser;
```

### Alternative Approaches for Non-Privileged Users

If you cannot grant file reading privileges, use these alternatives:

#### Load Kubeconfig via psql

```postgresql
-- Using psql variable substitution
\set kubeconfig `cat ~/.kube/config`
select omni_kube.load_kubeconfig(omni_yaml.to_json(:'kubeconfig'));
```

#### Manual Token Loading

```postgresql
-- Load token manually and set configuration
\set token `cat /var/run/secrets/kubernetes.io/serviceaccount/token`
\set cacert `cat /var/run/secrets/kubernetes.io/serviceaccount/ca.crt`

-- Set configuration manually
select set_config('omni_kube.token', :'token', false);
select set_config('omni_kube.cacert', :'cacert', false);
select set_config('omni_kube.server', 'https://kubernetes.default.svc', false);
```

## Cluster Configuration

All authentication methods also configure cluster connection details:

**Configuration Variables Set:**

- `omni_kube.server` - The Kubernetes API server URL
- `omni_kube.cacert` - The cluster CA certificate (PEM format)

### CA Certificate Handling

The system supports multiple ways to specify the cluster CA certificate:

```yaml
clusters:
- name: my-cluster
  cluster:
    server: https://kubernetes.example.com:6443
    # Option 1: Embedded base64-encoded certificate
    certificate-authority-data: LS0tLS1CRUdJTi...
    # Option 2: File reference (requires file system access)
    certificate-authority: /path/to/ca.crt
```

## Error Handling

The credential loading functions provide comprehensive error handling:

### Missing Context

```postgresql
-- Will raise an exception if context doesn't exist
select omni_kube.load_kubeconfig('/home/user/.kube/config', 'nonexistent-context');
-- ERROR: Context 'nonexistent-context' not found in kubeconfig
```

### Permission Errors

```postgresql
-- Will raise an exception if lacking file reading privileges
select omni_kube.load_kubeconfig('/home/user/.kube/config');
-- ERROR: permission denied to read file
```

### Unsupported Authentication Methods

```postgresql
-- For exec plugins or auth providers
select omni_kube.load_kubeconfig('/home/user/.kube/config', 'exec-context');
-- NOTICE: Exec plugin authentication detected but not yet supported
-- ERROR: Exec plugin authentication requires external command execution
```

### Missing Files

```postgresql
-- Will raise an exception if referenced files don't exist
select omni_kube.load_kubeconfig('/nonexistent/kubeconfig');
-- ERROR: could not open file "/nonexistent/kubeconfig"
```

## Configuration Variables

After successful credential loading, the following Postgres configuration variables are available:

| Variable                       | Description                  | Example                               |
|--------------------------------|------------------------------|---------------------------------------|
| `omni_kube.server`             | Kubernetes API server URL    | `https://kubernetes.example.com:6443` |
| `omni_kube.cacert`             | Cluster CA certificate (PEM) | `-----BEGIN CERTIFICATE-----\n...`    |
| `omni_kube.clientcert`         | Client certificate (PEM)     | `-----BEGIN CERTIFICATE-----\n...`    |
| `omni_kube.client_private_key` | Client private key (PEM)     | `-----BEGIN PRIVATE KEY-----\n...`    |
| `omni_kube.token`              | Bearer token                 | `eyJhbGciOiJSUzI1NiIs...`             |

These variables are automatically used by other omni_kube functions for API authentication.

## Security Considerations

- **File Permissions**: Ensure kubeconfig files have appropriate permissions (typically 600)
- **Postgres Privileges**: File reading functions require elevated privileges. Grant only `pg_read_server_files` when
  possible instead of superuser
- **Service Account Tokens**: In-cluster tokens are automatically rotated by Kubernetes
- **Configuration Scope**: Credentials are loaded at the session level and persist for the duration of the database
  connection
- **Sensitive Data**: Configuration variables containing sensitive data should be handled according to your security
  policies
- **Least Privilege**: Consider using manual credential loading via psql for environments where file system access
  should be restricted
