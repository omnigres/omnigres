The fastest way to try Omnigres out is by using
its [container image](https://github.com/omnigres/omnigres/pkgs/container/omnigres):

```shell
docker volume create omnigres
# The environment variables (`-e`) below have these set as defaults
docker run --name omnigres \
           -e POSTGRES_PASSWORD=omnigres \
           -e POSTGRES_USER=omnigres \
           -e POSTGRES_DB=omnigres \
           --mount source=omnigres,target=/var/lib/postgresql/data \
           -p 5432:5432 -p 8080:8080 --rm ghcr.io/omnigres/omnigres:latest
# Now you can connect to it:
psql -h localhost -p 5432 -U omnigres omnigres # password is `omnigres`
```

You can access the HTTP server at [localhost:8080](http://localhost:8080)

!!! tip "Important updates"

    __April 2023__: Omnigres container image now includes [plrust](https://github.com/tcdi/plrust), an extension that allows developing
    functions in Rust!

??? question "Which batteries are included?"

    This image contains all Omnigres extensions (with omni_httpd and omni_web preinstalled),
    as well as the following languages & extensions:
    
    * __plrust__ (That's right, you can use Rust!)
    * uuid-ossp
    * btree_gin
    * dict_int
    * hstore
    * lo
    * pg_prewarm
    * pg_walinspect
    * refint
    * tsm_system_rows
    * adminpack
    * btree_gist
    * dict_xsyn
    * insert_username
    * ltree
    * pg_stat_statements pgcrypto
    * seg
    * tsm_system_time
    * amcheck
    * citext
    * earthdistance
    * intagg
    * moddatetime
    * pageinspect
    * pg_surgery
    * pgrowlocks
    * sslinfo
    * unaccent
    * autoinc
    * cube
    * file_fdw
    * intarray
    * old_snapshot
    * pg_buffercache
    * pg_trgm
    * pgstattuple
    * tablefunc
    * xml2
    * bloom
    * dblink
    * fuzzystrmatch
    * isn
    * pg_freespacemap
    * pg_visibility
    * postgres_fdw
    * tcn

??? warning "Why is the container image so large?"

    Unfortunately, __plrust__ extension is responsible for many gigabytes of artifacts in the image. Typically,
    if you need Rust, this is not a big problem as both development machines and servers can handle
    this just fine.

    However, if you want a smaller image and don't need Rust, use __slim__ flavor:

    ```
    ghcr.io/omnigres/omnigres-slim:latest
    ```

### Building your own image

If you can't use the pre-built image (for example, you are running a fork or made changes), you can build the image
yourself:

```shell
# Build the image
DOCKER_BUILDKIT=1 docker build . -t ghcr.io/omnigres/omnigres
```
