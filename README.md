![Omnigres](header_logo.svg)
---

[![Discord Chat](https://img.shields.io/discord/1060568981725003789?label=Discord)][Discord]
[![Documentation](https://img.shields.io/badge/docs-ready-green)](https://docs.omnigres.org)
![License](https://img.shields.io/github/license/omnigres/omnigres)

Omnigres makes PostgreSQL a complete application platform. You can deploy a single database instance and it can host your entire application, scaling as needed.

* Running application logic **inside** or **next to** the database instance
* **Deployment** provisioning (**Git**, **containers**, etc.)
* Database instance serves **HTTP**, **WebSocket** and other protocols
* In-memory and volatile on-disk **caching**
* Routine application building blocks (**authentication**, **authorization**, **payments**, etc.)
* Database-modeled application logic via **reactive** queries
* Automagic remote **APIs** and **form** handling
* **Live** data updates

## Blogs and Publications

* [Omnigres maintainer's blog](https://yrashk.com/blog/category/omnigres/)

## :runner: Quick start

The fastest way to try Omnigres out is by using its [container image](https://github.com/omnigres/omnigres/pkgs/container/omnigres):

```shell
docker volume create omnigres
docker run --name omnigres --mount source=omnigres,target=/var/lib/postgresql/data \
           -p 5432:5432 -p 8080:8080 --rm ghcr.io/omnigres/omnigres:latest
# Now you can connect to it:
psql -h localhost -p 5432 -U omnigres omnigres # password is `omnigres`
```

Postgres parameters such as database, user or password can be overridden as per the
"Environment Variales" section in [postgres image instructions](https://hub.docker.com/_/postgres/)

You can access the HTTP server at [localhost:8080](http://localhost:8080)

### Building your own image

If you can't use the pre-built image (for example, you are running a fork or made changes), you can build the image yourself:

```shell
# Build the image
DOCKER_BUILDKIT=1 docker build . -t ghcr.io/omnigres/omnigres
```

## :wave: "Hello, world"

Here we expect you are running the [container image](#-runner--quick-start), which has
omni_httpd and omni_web extensions provisioned by default.

Let's start with a traditional example:

```sql
update omni_httpd.handlers
set
    query =
        $$select omni_httpd.http_response('Hello, world!') from request;$$;
```

Here we instruct the handler that is provisioned by omni_httpd by default
to use the enclosed query to greet the world:

```shell
$ curl localhost:8080
Hello, world!
```

Now, let's make it more personal and let it greet the requester by name.

```sql
update omni_httpd.handlers
set
    query =
        $$select omni_httpd.http_response('Hello, ' || 
                   coalesce(omni_web.param_get(request.query_string, 'name'), 'world') || '!')
          from request;$$;
```

Now, it'll respond in a personalized manner if `name` query string parameter is provided:

```shell
$ curl localhost:8080
Hello, world!

$ curl "localhost:8080?name=John"
Hello, John!
```

This, of course, only barely scratches the surface, but it may give you a very high-level concept
of how Omnigres web services can be built.

For a more complex example, that uses the underlying database and employs more real-world layout, check out
this [MOTD service example](https://docs.omnigres.org/examples/motd/).

## :building_construction: Component Roadmap

Below is the current list of components being worked on, experimented with and discussed. This list will change
(and grow) over time.

| Name                                                                                        | Status                                                                  | Description                                           |
|---------------------------------------------------------------------------------------------|-------------------------------------------------------------------------|-------------------------------------------------------|
| [omni_http](extensions/omni_http/README.md)                                                 | :white_check_mark: First release candidate                              | Common HTTP types library                             |
| [omni_httpd](extensions/omni_httpd/README.md) and [omni_web](extensions/omni_web/README.md) | :white_check_mark: First release candidate                              | Serving HTTP in Postgres and building services in SQL |
| [omni_mimetypes](extensions/omni_mimetypes/README.md)                                       | :white_check_mark: First release candidate                              | MIME types and file extensions                        |
| [omni_httpc](extensions/omni_httpc/README.md)                                               | :white_check_mark: First release candidate                              | HTTP client                                           |
| [omni_sql](extensions/omni_sql/README.md)                                                   | :construction: Extremely limited API surface                            | Programmatic SQL manipulation                         |
| [omni_containers](extensions/omni_containers/README.md)                                     | :ballot_box_with_check: Initial prototype                               | Managing containers                                   |
| [omni_ext](extensions/omni_ext/README.md) and  [Dynpgext interface](dynpgext/README.md)     | :ballot_box_with_check: Getting ready to become first release candidate | Advanced Postgres extension loader                    |
| omni_git                                                                                    | :lab_coat: Early experiments (unpublished)                              | Postgres Git client                                   |
| [omni_types](extensions/omni_types/README.md)                                               | :white_check_mark: First release candidate                              | Advanced Postgres typing techniques (sum types, etc.) |
| [omni_seq](extensions/omni_seq/README.md)                                                   | :white_check_mark: First release candidate                              | Extended Postgres sequence tooling                    |
| omni_reactive                                                                               | :spiral_calendar: Haven't started yet                                   | Reactive queries                                      |

## :keyboard: Hacking

## Building & using extensions

To build and run Omnigres, you would currently need a recent C compiler, OpenSSL and cmake:

```shell
mkdir -p build && cd build
cmake ..
make psql_<COMPONENT_NAME> # for example, `psql_omni_containers`
```

### Running tests

```shell
# in the build directory
CTEST_PARALLEL_LEVEL=$(nproc) make -j $(nproc) all test
```

[Discord]: https://discord.omnigr.es
