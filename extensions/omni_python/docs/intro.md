# Intro

`omni_python` allows you to seamlessly integrate Python code within Omnigres, which are then used as stored procedures in the database. `omni_python` also allows you to integrate Flask framework and serve HTTP requests from directly within Omnigres.

## Prerequisites

Create `omni_python` extension if it is not installed.
```postgresql
create extension if not exists omni_python cascade;
```

We would also need some helper extensions for loading relevant files from filesystem.
```postgresql
create extension if not exists omni_schema cascade;
create extension if not exists omni_vfs cascade;
```

## Getting started

1. Create a directory on your host system containing the Python files, and corresponding `requirements.txt`, and mount the volume to the Docker container.

2. We need to add the following to `requirements.txt` file to support Python integration:
```shell
omni_python
```

3. Create Python files. You can create more than one files, as long as they are in the directory that you would mount as a volume inside the Docker container for running Omnigres.

Create functions in Python as you would, and annotate them with `@pg` to make sure they are loaded into the database.

For example, let's make two Python files.

`hello.py`:
```python
from omni_python import pg

@pg
def hello() -> str:
    return "Hey there!"
```

`maths.py`:
```python
from omni_python import pg

@pg
def add(a: int, b: int) -> int:
    return a + b

@pg
def subtract(a: int, b: int) -> int:
    return a - b
```

4. Create a function to define an identifier for the local directory you want to load into the database. For the example, the local directory is `python-files`.

```postgresql
create or replace function demo_function() returns omni_vfs.local_fs language sql
as $$
select omni_vfs.local_fs('/python-files')
$$;
```

5. Configure `omni_python`
```postgresql
insert into omni_python.config (name, value) values ('pip_find_links', '/python-wheels');
```

6. Load the filesystem files.
```postgresql
select omni_schema.load_from_fs(demo_function());
```

7. [Optional] You can set a reload command for reloading the filesystem changes.
```postgresql
\set reload 'select omni_schema.load_from_fs(demo_function());'
```

8. Run Omnigres in a Docker container. Make sure to mount the local directory as a volume on the correct path.
```shell
docker run --name omnigres \
           -e POSTGRES_PASSWORD=omnigres \
           -e POSTGRES_USER=omnigres \
           -e POSTGRES_DB=omnigres \
           --mount source=omnigres,target=/var/lib/postgresql/data -v $(pwd)/python-files:/python-files \
           -p 127.0.0.1:5433:5432 --rm ghcr.io/omnigres/omnigres-slim:latest
```

9. Let's try it out!
```postgresql
omnigres=# select hello();
   hello
------------
 Hey there!
```
```postgresql
omnigres=# select add(5, 10);
 add
-----
  15
```
```postgresql
omnigres=# select subtract(5, 10);
 subtract
----------
       -5
```

## Flask

For Flask framework integration, we have a few more steps.

1. Create `omni_httpd` extension to be able to handle HTTP requests.
```postgresql
create extension if not exists omni_httpd cascade;
```
2. Add the following in `requirements.txt` file
```
omni_http[Flask]
```

3. Create HTTP listeners for our Flask application. For example, let's add a HTTP listener for port 5000.
```postgresql
with
    listener as (insert into omni_httpd.listeners (address, port) values ('0.0.0.0', 5000) returning id),
    handler as (insert into omni_httpd.handlers (query) values
                                                 (
                                                 $$ select handle(request.*) from request$$
       ) returning id)
insert
into
    omni_httpd.listeners_handlers (listener_id, handler_id)
select
    listener.id,
    handler.id
from
    listener,
    handler;
```

4. Now you can update your Python files (in the mounted volume) to include Flask functionality.
```python
from omni_python import pg
from omni_http import omni_httpd
from omni_http.omni_httpd import flask
from flask import Flask, jsonify, make_response

app = Flask('myapp')

@app.route("/")
def ping():
    return "<h1>Hello, World!</h1>"

@app.route('/post/<int:post_id>', methods=['POST'])
def show_post(post_id):
    resp = make_response(f'<h1>Post #{post_id}</h1>')
    resp.set_cookie('cookie', 'yum')
    return resp

@app.route("/test.json")
def json():
    return jsonify({"test": "passed"})

app_ = flask.Adapter(app)

@pg
def handle(req: omni_httpd.HTTPRequest) -> omni_httpd.HTTPOutcome:
    return app_(req)
```

5. Make sure to add port mapping for 5000 when running Omnigres via Docker.
```shell
docker run --name omnigres \
           -e POSTGRES_PASSWORD=omnigres \
           -e POSTGRES_USER=omnigres \
           -e POSTGRES_DB=omnigres \
           --mount source=omnigres,target=/var/lib/postgresql/data -v $(pwd)/python-files:/python-files \
           -p 127.0.0.1:5450:5432 -p 127.0.0.1:5000:5000 --rm ghcr.io/omnigres/omnigres-slim:latest
```

6. You can hit the endpoints defined in the Flask code above.

```shell
$ curl http://localhost:5000
<h1>Hello, World!</h1>
```

```shell
$ curl -X POST http://localhost:5000/post/123
<h1>Post #123</h1>
```

```shell
$ curl http://localhost:5000/test.json
{"test":"passed"}
```
