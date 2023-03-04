![Omnigres](header_logo.svg)
---

[![Discord Chat](https://img.shields.io/discord/1060568981725003789?label=Discord)][Discord]
[![Telegram Group](https://img.shields.io/endpoint?color=neon&style=flat-square&url=https%3A%2F%2Ftg.sumanjay.workers.dev%2Fomnigres)][Telegram]

Omnigres makes PostgreSQL a complete application platform. You can deploy a single database instance and it can host your entire application, scaling as needed.

* Running application logic **inside** or **next to** the database instance
* **Deployment** provisioning (**Git**, **Docker**, etc.)
* Database instance serves **HTTP**, **WebSocket** and other protocols
* In-memory and volatile on-disk **caching**
* Routine application building blocks (**authentication**, **authorization**, **payments**, etc.)
* Database-modeled application logic via **reactive** queries
* Automagic remote **APIs** and **form** handling
* **Live** data updates

*This is a working-in-progress project at the moment and not ready for any kind of production use. If anything, it's at the prototyping stage.*

## Components

* [omni_containers](extensions/omni_containers/README.md)
* [omni_ext](extensions/omni_ext/README.md)
* [omni_sql](extensions/omni_sql/README.md)
* [omni_httpd](extensions/omni_httpd/README.md)
* [Dynpgext interface](dynpgext/README.md)

## Quick start

The fastest way to try Omnigres out is by using
its [Docker image](https://github.com/omnigres/omnigres/pkgs/container/omnigres):

```shell
docker volume create omnigres
docker run -d --name omnigres -e POSTGRES_PASSWORD=omnigres -e POSTGRES_USER=omnigres \
                              -e POSTGRES_DB=omnigres --mount source=omnigres,target=/var/lib/postgresql/data \
              -p 5432:5432 ghcr.io/omnigres/omnigres:master
# Now you can connect to it:
psql -h localhost -p 5432 -U omnigres omnigres # password is `omnigres`
````

### Building your own image

If you can't use the pre-built image (for example, you are running a fork or made changes), you can build the image
yourself:

```shell
# Build the image
DOCKER_BUILDKIT=1 docker build . -t omnigres
```

## Hacking

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

[Discord]: https://discord.gg/Jghrq588qS
[Telegram]: https://telegram.dog/omnigres
