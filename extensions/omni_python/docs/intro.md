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

4. Let's say you have a table called `employees`.
```postgresql
create table employees (
   id uuid,
   name text not null,
   department text not null,
   salary integer not null
);
```

5. Now you can update your Python files (in the mounted volume) to include Flask functionality. For example, you can define endpoints to fetch list of all employees, fetch a particular employee, as well as create a new employee record.
```python
from omni_python import pg
from omni_http import omni_httpd
from omni_http.omni_httpd import flask
from flask import Flask, jsonify, make_response
import uuid

app = Flask('myapp')

@app.route('/employees', methods=['POST'])
def create_employee():
    employee_id = uuid.uuid4()
    # todo: Use actual data from request body once https://github.com/omnigres/omnigres/pull/316 is merged.
    employee = plpy.execute(plpy.prepare("INSERT INTO employees (id, name, department, salary) VALUES ($1, $2, $3, $4) RETURNING *", ["uuid", "text", "text", "int"]), [employee_id, "John Doe", "Engineering", 100000])
    return str(employee)

@app.route('/employees', methods=['GET'])
def get_employees():
    employees = plpy.execute(plpy.prepare("SELECT * FROM employees"))
    return str(employees)

@app.route('/employees/<employee_id>', methods=['GET'])
def get_employee(employee_id):
    employee = plpy.execute(plpy.prepare("SELECT * FROM employees WHERE id = $1", ["uuid"]), [employee_id])
    return str(employee)

app_ = flask.Adapter(app)

@pg
def handle(req: omni_httpd.HTTPRequest) -> omni_httpd.HTTPOutcome:
    return app_(req)
```

!!! tip "Tip"

    Note: We use `plpy` for now, but we should use DB API compatible APIs and/or other 
    frameworks (such as SQLAlchemy) which will be available very soon.

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

Fetch all employees:
```shell
$ curl http://localhost:5000/employees
<PLyResult status=5 nrows=2 rows=[
{'id': '3f59ef0a-1f57-405a-87d4-662f16698a72', 'name': 'Akshat', 'department': 'Engineering', 'salary': 200000}, 
{'id': '77e52508-829e-4004-9ed5-7e386bb18d76', 'name': 'Daniel', 'department': 'Engineering', 'salary': 100000}
]>
```

Create a new employee:
```shell
$ curl -X POST http://localhost:5000/employees
<PLyResult status=11 nrows=1 rows=[
{'id': '8b26fcf2-4ba8-4b4a-8e9c-1bb08d0695e4', 'name': 'John Doe', 'department': 'Engineering', 'salary': 100000}
]>
```

Fetch a particular employee:
```shell
$ curl http://localhost:5000/employees/8b26fcf2-4ba8-4b4a-8e9c-1bb08d0695e4
<PLyResult status=5 nrows=1 rows=[
{'id': '8b26fcf2-4ba8-4b4a-8e9c-1bb08d0695e4', 'name': 'John Doe', 'department': 'Engineering', 'salary': 100000}
]>
```
