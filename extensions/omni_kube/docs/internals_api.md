# Internals: Kubernetes API

## `omni_kube.api()`

This is the central function to invoke Kubernetes API calls

|  **Parameter** | **Type**                      | **Description**                                                 |
|---------------:|-------------------------------|-----------------------------------------------------------------|
|       **path** | text                          | Request path                                                    |
|     **server** | text                          | Kubernetes server, defaults to `https://kubernetes.default.svc` |
|     **cacert** | text                          | CA certificate                                                  |
| **clientcert** | omni_httpc.client_certificate | Client certificate                                              |
|      **token** | text                          | Bearer token                                                    |
|     **method** | omni_http.http_method         | HTTP method, defaults to `GET`                                  |
|       **body** | jsonb                         | Request body                                                    |

**token** and **cacert** are automatically inferred from default pod's
paths (`var/run/secrets/kubernetes.io/serviceaccount/token` and `/var/run/secrets/kubernetes.io/serviceaccount/ca.crt`
respectively) to enable
seamless use of API from within pods (through `omni_kube.pod_credentials()` function). They can be overriden by
corresponding
function parameters or `omni_kube.token` and `omni_kube.cacert` settings. In addition
`omni_kube.clientcert` and `omni_kube.client_private_key` settings can be used to override
the `clientcert` parameter.
