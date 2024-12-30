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

??? warning "Why is the container image so large?"

    We included a significant amount extension, currently sourced from [Pigsty](https://pigsty.io/)

    However, if you want a smaller image and don't need all of them, use the __slim__ flavor:

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
