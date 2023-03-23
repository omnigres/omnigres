![Omnigres](header_logo.svg)
---

[![Discord Chat](https://img.shields.io/discord/1060568981725003789?label=Discord)][Discord]

Omnigres makes PostgreSQL a complete application platform. You can deploy a single database instance and it can host your entire application, scaling as needed.

* Running application logic **inside** or **next to** the database instance
* **Deployment** provisioning (**Git**, **containers**, etc.)
* Database instance serves **HTTP**, **WebSocket** and other protocols
* In-memory and volatile on-disk **caching**
* Routine application building blocks (**authentication**, **authorization**, **payments**, etc.)
* Database-modeled application logic via **reactive** queries
* Automagic remote **APIs** and **form** handling
* **Live** data updates

## :runner: Quick start

The fastest way to try Omnigres out is by using
its [container image](https://github.com/omnigres/omnigres/pkgs/container/omnigres):

```shell
docker volume create omnigres
docker run --name omnigres -e POSTGRES_PASSWORD=omnigres -e POSTGRES_USER=omnigres \
                           -e POSTGRES_DB=omnigres --mount source=omnigres,target=/var/lib/postgresql/data \
           -p 5432:5432 -p 8080:8080 --rm ghcr.io/omnigres/omnigres:latest
# Now you can connect to it:
psql -h localhost -p 5432 -U omnigres omnigres # password is `omnigres`
````

You can access the HTTP server at [localhost:8080](http://localhost:8080)

### Building your own image

If you can't use the pre-built image (for example, you are running a fork or made changes), you can build the image
yourself:

```shell
# Build the image
DOCKER_BUILDKIT=1 docker build . -t ghcr.io/omnigres/omnigres
```

## :wave: "Hello, world"

Below is a simple web application that runs inside of Postgres
and manages [MOTD (Message Of The Day)](https://en.wikipedia.org/wiki/Message_of_the_day).

```sql
create table if not exists motd
(
    id        int primary key generated always as identity,
    content   text,
    posted_at timestamp default now()
);

create or replace function show_motd() returns setof omni_httpd.http_response as
$$
select
    omni_httpd.http_response(body => 'Posted at ' || posted_at || E'\n' || content)
from
    motd
order by
    posted_at desc
limit 1;
$$ language sql;

create or replace function no_motd() returns setof omni_httpd.http_response as
$$
select omni_httpd.http_response(body => 'No MOTD');
$$
    language sql;

create or replace function update_motd(request omni_httpd.http_request) returns omni_httpd.http_response as
$$
insert
into
    motd (content)
values
    (convert_from(request.body, 'UTF8'))
returning omni_httpd.http_response(status => 201);
$$
    language sql;

update omni_httpd.handlers
set
    query = (select
                 omni_httpd.cascading_query(name, query order by priority desc nulls last)
             from
                 (values
                      ('show', $$select show_motd() from request where request.method = 'GET'$$, 1),
                      ('update', $$select update_motd(request.*) from request where request.method = 'POST'$$, 1),
                      ('fallback', $$select no_motd() from request where request.method = 'GET'$$,
                       0)) handlers(name, query, priority));
```

It works like this:

```shell
GET / # => HTTP/1.1 200 OK
No MOTD

POST / "Check out Omnigres" # => HTTP/1.1 201 OK

GET / # => HTTP/1.1 200 OK
Posted at 2023-03-23 02:59:14.679113
Check out Omnigres
```

All you need to run this is just an instance of Postgres with
Omnigres extensions installed, like the one in the [container image](#quick-start).

## :building_construction: Component Roadmap

Below is the current list of components being worked on, experimented with and discussed. This list will change
(and grow) over time.

| Name                                                                                        | Status                                                                  | Description                                           |
|---------------------------------------------------------------------------------------------|-------------------------------------------------------------------------|-------------------------------------------------------|
| [omni_httpd](extensions/omni_httpd/README.md) and [omni_web](extensions/omni_web/README.md) | :white_check_mark: First release candidate                              | Serving HTTP in Postgres and building services in SQL |
| omni_httpclient                                                                             | :spiral_calendar: Haven't started yet                                   | Postgres HTTP client                                  |
| [omni_sql](extensions/omni_sql/README.md)                                                   | :construction: Extremely limited API surface                            | Programmatic SQL manipulation                         |
| [omni_containers](extensions/omni_containers/README.md)                                     | :ballot_box_with_check: Initial prototype                               | Managing containers                                   |
| [omni_ext](extensions/omni_ext/README.md) and  [Dynpgext interface](dynpgext/README.md)     | :ballot_box_with_check: Getting ready to become first release candidate | Advanced Postgres extension loader                    |
| omni_git                                                                                    | :lab_coat: Early experiments (unpublished)                              | Postgres Git client                                   |
| omni_types                                                                                  | :lab_coat: Early experiments (unpublished)                              | Advanced Postgres typing techniques (sum types, etc.) |
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

[Discord]: https://discord.gg/Jghrq588qS
