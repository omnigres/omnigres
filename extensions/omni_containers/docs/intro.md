# Intro

`omni_containers` manages containers that can be automatically connected to the database instance and their lifecycle can be managed from within
the database.

## Prerequisites
Docker engine API should be accessible to the PostgreSQL instance which is configurable through the env variable `DOCKER_HOST` if not set it defaults to unix domain socket at `/var/run/docker.sock`

## Create container
Create and start a container with `busybox` image(pull if not already present) and run `echo Hello world` command in it

```postgresql
select * from omni_containers.docker_container_create('busybox', cmd => 'echo Hello world', pull => true);
```

Returns the container id as the result
```psql
NOTICE:  Pulling image busybox
                     docker_container_create                      
------------------------------------------------------------------
 9e7b911de036955b13752d740597aa85187ed520d7f4aead128a88328df6e923
(1 row)
```

|        Parameter | Type                   | Description                      | Default     |
|-----------------:|------------------------|----------------------------------|-------------|
|        **image** | text                   | image name                       | None        | 
|          **cmd** | text                   | command to run                   | `NULL`      | 
|       **attach** | text                   | attach local PostgreSQL as       | db.omni     | 
|        **start** | boolean                | start the container              | true        |
|         **wait** | boolean                | wait till container exits        | false       |
|         **pull** | boolean                | pull if image absent             | false       |
|      **options** | jsonb                  | extra options                    | {}          |


## Query container logs
Read the logs of container with a given id
```postgresql
select * from omni_containers.docker_container_logs('9e7');
```
```psql
 docker_container_logs 
-----------------------
 Hello world          +
 
(1 row)
```

|        Parameter | Type                        | Description               | Default     |
|-----------------:|-----------------------------|---------------------------|-------------|
|        **id**    | text                        | container id              | None        | 
|       **stdout** | bool                        | include stdout            | true        | 
|       **stderr** | bool                        | include stderr            | true        | 
|        **since** | timestamp without time zone | logs since                | `NULL`      |
|        **until** | timestamp without time zone | logs until                | `NULL`      |
|   **timestamps** | boolean                     | add timestamps to log     | false       |
|         **tail** | integer                     | num of log lines from end | `NULL`      |

## Attach postgres to container
Create a container which connects and sends queries to the postgresql instance

```postgresql
with container as (select * from omni_containers.docker_container_create('ghcr.io/yrashk/psql',
        cmd => $$psql -c 'create table container_test ();'$$,
        wait => true, pull => true) as id)
    select
        omni_containers.docker_container_inspect(id) -> 'State' -> 'ExitCode' as exit_code,
        omni_containers.docker_container_logs(id)  as logs
    from
container;
```

```psql
exit_code |     logs     
-----------+--------------
 0         | CREATE TABLE+
           | 
(1 row)
```

## Stop container
Stop the container with given id
```postgresql
select * from omni_containers.docker_container_stop('9e7');
```

|        Parameter | Type                        | Description               | Default     |
|-----------------:|-----------------------------|---------------------------|-------------|
|        **id**    | text                        | container id              | None        | 

## List images
List the locally available repo_tags using `docker images` view
```postgresql
select repo_tags from docker_images;
```
```psql
          repo_tags           
------------------------------
 {minio/minio:latest}
 {alpine:latest}
 {busybox:latest}
 {ghcr.io/yrashk/psql:latest}
(4 rows)
```

View schema

|           Column | Type                     |
|-----------------:|--------------------------|
| **id**           | text                     |
| **size**         | bigint                   |
| **labels**       | jsonb                    |
| **created_at**   | timestamp with time zone |
| **parent_id**    | text                     |
| **repo_tags**    | text[]                   |
| **containers**   | integer                  |
| **shared_size**  | integer                  |
| **repo_digests** | text[]                   |
| **virtual_size** | jsonb                    |


## Inspect container
Returns the entire inspect output as `jsonb`

Check the exit code of container with given id
```postgresql
select info->'State'->'ExitCode' as exit_code from docker_container_inspect('9e7') as info;
```
```psql
 exit_code 
-----------
 0
(1 row)
```

|        Parameter | Type                        | Description               | Default     |
|-----------------:|-----------------------------|---------------------------|-------------|
|        **id**    | text                        | container id              | None        | 