<p align="center">
<h1 align="center">Omnigres</h1>
<picture>
<source media="(prefers-color-scheme: dark)" srcset="arch_dark.svg">
<img src="arch.svg" alt="Omnigres Architecture">
</picture>
</p>

<p align="center">
<a href="https://docs.omnigres.org"><b>Documentation</b></a> |
<a href="https://github.com/omnigres/omnigres/wiki/Bounties"><b>Bounties</b></a>
</p>

---

[![Discord Chat](https://img.shields.io/discord/1060568981725003789?label=Discord)][Discord]
[![Documentation](https://img.shields.io/badge/docs-ready-green)](https://docs.omnigres.org)
![License](https://img.shields.io/github/license/omnigres/omnigres)

Omnigres makes Postgres a developer-first application platform. You can deploy a single database instance and it can host your entire application, scaling as needed.

* Running application logic **inside** or **next to** the database instance
* **Deployment** provisioning (**Git**, **containers**, etc.)
* Database instance serves **HTTP**, **WebSocket** and other protocols
* In-memory and volatile on-disk **caching**
* Routine application building blocks (**authentication**, **authorization**, **payments**, etc.)
* Database-modeled application logic via **reactive** queries
* Automagic remote **APIs** and **form** handling
* **Live** data updates

## Blogs and Publications

* [Omnigres blog](https://blog.omnigres.com)

## :runner: Quick start

The fastest way to try Omnigres out is by using its [container image](https://github.com/omnigres/omnigres/pkgs/container/omnigres):

```shell
docker volume create omnigres
docker run --name omnigres --mount source=omnigres,target=/var/lib/postgresql/data \
           -p 127.0.0.1:5432:5432 -p 127.0.0.1:8080:8080 -p 127.0.0.1:8081:8081 --rm ghcr.io/omnigres/omnigres-17:latest
# Now you can connect to it:
psql -h localhost -p 5432 -U omnigres omnigres # password is `omnigres`
```

> [!TIP]
> Replace `ghcr.io/omnigres/omnigres-17` with `ghcr.io/omnigres/omnigres-extra-17` if you want an image with a lot more batteries included.

Postgres parameters such as database, user or password can be overridden as per the
"Environment Variables" section in [postgres image instructions](https://hub.docker.com/_/postgres/)

You can access the default HTTP server at [localhost:8081](http://localhost:8081)

### Building your own image

If you can't use the pre-built image (for example, you are running a fork or made changes), you can build the image yourself:

```shell
# Build the image
DOCKER_BUILDKIT=1 docker build . -t ghcr.io/omnigres/omnigres
```

## Download omnigres extensions

Omnigres extensions can also
be [downloaded and installed](https://docs.omnigres.org/omni_manifest/usage/#download-omnigres-extensions)
in any postgres installation with file system access.

## :wave: "Hello, world"

Here we expect you are running the [container image](#-runner--quick-start), which has
omni_httpd and omni_web extensions provisioned by default.

Let's start with a traditional example. Here we will instruct the handler that
is provisioned by omni_httpd by default to use the enclosed query to greet the
world.

Below, we'll show examples in Python and plain SQL (or PL/pgSQL). Support for
more languages is coming!

```shell
$ curl localhost:8081
Hello, world!
```

<details>
<summary>Python (Flask) implementation</summary>

```python
from omni_python import pg
from flask import Flask
from omni_http.omni_httpd import flask

app = Flask('myapp')


@app.route('/')
def hello():
    return "Hello, world!"


handle = pg(flask.Adapter(app))
```

To connect the endpoint:

```sql
insert into omni_httpd.urlpattern_router (match, handler)
values (omni_httpd.urlpattern('/'), 'my_handler'::regproc);
```

**NB**: Please note that you will need to
[follow Python setup steps](https://docs.omnigres.org/omni_python/intro/)
for the time being before our CLI tooling is ready.

</details>

<details>
<summary>Plain SQL</summary>

You can also achieve the same using plain SQL with very little setup.

```sql
create function my_handler()
  returns omni_httpd.http_outcome
  return omni_httpd.http_response('Hello world!');

insert into omni_httpd.urlpattern_router (match, handler)
values (omni_httpd.urlpattern('/'), 'my_handler'::regproc);
```
</details>

Now, let's make it more personal and let it greet the requester by name.

```shell
$ curl "localhost:8081?name=John"
Hello, John!
```

<details>
<summary>Python (Flask) implementation</summary>

```python
from flask import request  # we need to access `request`


@app.route('/')
def hello():
    return f"Hello, {request.args.get('name', 'world')}!"
```

</details>

<details>
<summary>Plain SQL</summary>

```sql
update omni_httpd.handlers
set
    query =
        $$select omni_httpd.http_response('Hello, ' || 
                   coalesce(omni_web.param_get(request.query_string, 'name'), 'world') || '!')
          from request$$;
```

</details>

This, of course, only barely scratches the surface, but it may give you a very high-level concept
of how Omnigres web services can be built.

For a more complex example, that uses the underlying database and employs more real-world layout, check out
this [MOTD service example](https://docs.omnigres.org/examples/motd/).

## Omnigres Components

A collection of extensions that improve Postgres and turn it into a comprehensive business runtime.

<details>
<summary><strong>Application Management</strong></summary>

- **[omni_credentials](extensions/omni_schema/README.md)** - Application credential management
- **[omni_service](extensions/omni_service/README.md)** - Uniform service management bus
- **[omni_schema](extensions/omni_schema/README.md)** - Application schema management

</details>


<details>
<summary><strong>Extension system</strong></summary>

- **[omni](extensions/omni/README.md) and [Omni interface](omni/README.md)** - Advanced adapter for Postgres extensions
- **[omni_manifest](extensions/omni_manifest/README.md)** - Improved extension installation
- **[omni_service](extensions/omni_service/README.md)** - Uniform service management bus

</details>

<details>
<summary><strong>Application Stack</strong></summary>

- **[omni_auth](extensions/omni_auth/README.md)** - Authentication management
- **[omni_email](extensions/omni_email/README.md)** - Email management
- **[omni_datasets](extensions/omni_dataset/README.md)** - Dataset provider

</details>


<details>
<summary><strong>Fintech Stack</strong></summary>

- **omni_audit** (⏳ pending release) - Robust in-database audit & record change trail
- **[omni_ledger](extensions/omni_ledger/README.md)** - Financial transaction management

</details>


<details>
<summary><strong>Postgres enhancements</strong></summary>

- **[omni](extensions/omni/README.md) and [Omni interface](omni/README.md)** - Advanced adapter for Postgres extensions
- **[omni_polyfill](extensions/omni_polyfill/README.md)** - Provides polyfills for older versions of Postgres
- **[omni_worker](extensions/omni_worker/README.md)** - Generalized worker architecture

</details>


<details>
<summary><strong>Data Types</strong></summary>

- **[omni_id](extensions/omni_id/README.md)** - Identity types
- **[omni_types](extensions/omni_types/README.md)** - Advanced Postgres typing techniques (sum types, etc.)
- **[omni_seq](extensions/omni_seq/README.md)** - Extended Postgres sequence tooling
- **[omni_regex](extensions/omni_python/README.md)** - Feature-rich regular expressions

</details>

<details>
<summary><strong>Data Formats</strong></summary>

- **[omni_csv](extensions/omni_csv/README.md)** - CSV toolkit
- **[omni_json](extensions/omni_json/README.md)** - JSON toolkit
- **[omni_sqlite](extensions/omni_sqlite/README.md)** - Embedded SQLite
- **[omni_xml](extensions/omni_xml/README.md)** - XML toolkit
- **[omni_yaml](extensions/omni_yaml/README.md)** - YAML toolkit

</details>

<details>
<summary><strong>HTTP & APIs</strong></summary>

- **[omni_http](extensions/omni_http/README.md)** - Common HTTP types library
- **[omni_httpc](extensions/omni_httpc/README.md)** - HTTP client
- **[omni_httpd](extensions/omni_httpd/README.md)** & **[omni_web](extensions/omni_web/README.md)** - Serving HTTP in
  Postgres
- **[omni_mimetypes](extensions/omni_mimetypes/README.md)** - MIME types and file extensions
- **[omni_session](extensions/omni_session/README.md)** - Session management
- **[omni_rest](extensions/omni_rest/README.md)** - Out-of-the-box REST API provider

</details>

<details>
<summary><strong>Infrastructure</strong></summary>

- **[omni_aws](extensions/omni_aws/README.md)** - AWS APIs
- **[omni_containers](extensions/omni_containers/README.md)** - Managing containers
- **omni_cluster** (⏳ pending release) - Postgres High Availability as an extension
- **[omni_cloudevents](extensions/omni_cloudevents/README.md)** - [CloudEvents](https://cloudevents.io/) support
- **[omni_kube](extensions/omni_kube/README.md)** - Kubernetes integration

</details>

<details>
<summary><strong>Observability</strong></summary>

- **[omni_cloudevents](extensions/omni_cloudevents/README.md)** - [CloudEvents](https://cloudevents.io/) support

</details>

<details>
<summary><strong>Libraries</strong></summary>

- **[omni_regex](extensions/omni_python/README.md)** - Feature-rich regular expressions
- **[omni_sql](extensions/omni_sql/README.md)** - Programmatic SQL manipulation
- **[omni_txn](extensions/omni_txn/README.md)** - Transaction management
- **[omni_var](extensions/omni_var/README.md)** - Variable management

</details>


<details>
<summary><strong>Development Tools</strong></summary>

- **[omni_sql](extensions/omni_sql/README.md)** - Programmatic SQL manipulation
- **[omni_test](extensions/omni_test/README.md)** - Testing framework
- **[omni_datasets](extensions/omni_dataset/README.md)** - Dataset provider

</details>

<details>
<summary><strong>Language Integration</strong></summary>

- **[omni_python](extensions/omni_python/README.md)** - First-class Python Development Experience
- **[omni_codon](extensions/omni_codon/README.md)** - Pythonic compiled language ("Python, Turbocharged")

</details>

<details>
<summary><strong>Storage</strong></summary>

- **[omni_vfs](extensions/omni_vfs/README.md)** - Virtual File System interface

</details>

<details>
<summary><strong>System Integration</strong></summary>

- **[omni_os](extensions/omni_os/README.md)** - Access to the operating system

</details>

## :keyboard: Hacking

## Building & using extensions

To build and run Omnigres, you would need:

* a recent C compiler
* OpenSSL 3.2 (**optional**, will be built if not available)
* cmake >= 3.25.1
* (optionally, to use omni_containers or run a full set of tests) a recent
  version of Docker

<details>
<summary>Dependencies for Fedora</summary>

* Packages: `git cmake gcc g++ cpan openssl-devel openssl-devel-engine python-devel openssl bison flex readline-devel zlib-devel netcat`
* CMake flags: `-DOPENSSL_CONFIGURED=1`

</details>

<details>
<summary>Dependencies for macOS</summary>

* XCode Command Line Tools: `xcode-select --install`
* Homebrew packages: `cmake openssl python`

</details>

```shell
cmake -S . -B build
cmake --build build --parallel
cd build && make -j psql_<COMPONENT_NAME> # for example, `psql_omni_id`
```

To install extensions into your target Postgres:

```shell
cmake --build build --parallel --target install_extensions
# Or, individually,
cmake --build build --parallel --target install_<COMPONENT_NAME>_extension
```

#### Building a subset of extensions

One can pass an exclusion list or an explicit inclusion list to the first `cmake` step:

```shell
cmake -S . -B build -DOMNIGRES_EXCLUDE="omni_txn;omni_xml"
cmake -S . -B build -DOMNIGRES_INCLUDE="omni_httpd"
```

By default, all modules are included. Please note that if an extension is effectively excluded and is required
by an included one, `cmake` will fail with an error like this:

```
omni_http required by omni_httpc but is effectively excluded
```

### Troubleshooting

<details>
<summary>cmake not picking up Python version you want?</summary>

To use a specific Python build use the cmake flag `Python3_EXECUTABLE`:

```
cmake -S . -B build -DPython3_EXECUTABLE=/path/to/python
```

</details>

<details>
<summary>Build fails for whatever other reason?</summary>

Remove `build` and `.pg` directories for a clean rebuild:

```
rm -rf .pg build
```

</details>

### Running tests

```shell
# in the build directory
CTEST_PARALLEL_LEVEL=$(nproc) make -j $(nproc) all test
```

### Contributing

Once you are ready to contribute, please check out the [contribution guidelines](CONTRIBUTING.md).


[Discord]: https://discord.omnigr.es
