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
           -p 127.0.0.1:5432:5432 -p 127.0.0.1:8080:8080 --rm ghcr.io/omnigres/omnigres-17:latest
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
    
    * adminpack
    * amcheck
    * autoinc
    * bloom
    * bool_plperl
    * bool_plperlu
    * btree_gin
    * btree_gist
    * citext
    * cube
    * dblink
    * dict_int
    * dict_xsyn
    * earthdistance
    * file_fdw
    * fuzzystrmatch
    * hstore
    * hstore_plperl
    * hstore_plperlu
    * hstore_plpython3u
    * insert_username
    * intagg
    * intarray
    * isn
    * jsonb_plperl
    * jsonb_plperlu
    * jsonb_plpython3u
    * lo
    * ltree
    * ltree_plpython3u
    * moddatetime
    * old_snapshot
    * omni_containers
    * omni_ext
    * omni_http
    * omni_httpc
    * omni_httpd
    * omni_seq
    * omni_sql
    * omni_types
    * omni_web
    * pageinspect
    * pg_buffercache
    * pg_freespacemap
    * pg_prewarm
    * pg_stat_statements
    * pg_surgery
    * pg_trgm
    * pg_visibility
    * pg_walinspect
    * pgcrypto
    * pgrowlocks
    * pgstattuple
    * pljava _(temporarily excluded)_
    * plperl
    * plperlu
    * plpgsql
    * plpython3u
    * plrust
    * pltcl
    * pltclu
    * postgres_fdw
    * refint
    * seg
    * sslinfo
    * tablefunc
    * tcn
    * tsm_system_rows
    * tsm_system_time
    * unaccent
    * uuid-ossp
    * xml2

??? warning "Why is the container image so large?"

    Unfortunately, __plrust__ extension is responsible for many gigabytes of artifacts in the image. Typically,
    if you need Rust, this is not a big problem as both development machines and servers can handle
    this just fine.

    However, if you want a smaller image and don't need Rust, use __slim__ flavor:

    ```
    ghcr.io/omnigres/omnigres-slim-17:latest
    ```

### Building your own image

If you can't use the pre-built image (for example, you are running a fork or made changes), you can build the image
yourself:

```shell
# Build the image
DOCKER_BUILDKIT=1 docker build . -t ghcr.io/omnigres/omnigres
```
