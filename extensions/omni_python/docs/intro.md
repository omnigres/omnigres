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

Create a directory on your host system containing the Python files, and corresponding `requirements.txt`, and mount the volume to the Docker container.

We need to add the following to `requirements.txt` file to support Python integration:
```shell
omni_python
```

Create Python files. You can create more than one files, as long as they are in the directory that you would mount as a volume inside the Docker container for running Omnigres.

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
Create a function to define an identifier for the local directory you want to load into the database. For the example, the local directory is `python-files`.

```postgresql
create or replace function demo_function() returns omni_vfs.local_fs language sql
as $$
select omni_vfs.local_fs('/python-files')
$$;
```
Configure `omni_python`
```postgresql
insert into omni_python.config (name, value) values ('pip_find_links', '/python-wheels');
```
!!! tip "Tip"

    We are working on a CLI tooling that will take care of directory mapping.

Load the filesystem files.
```postgresql
select omni_schema.load_from_fs(demo_function());
```

!!! tip "Optional Tip"

    You can set a reload command for reloading the filesystem changes.
    ```postgresql
    \set reload 'select omni_schema.load_from_fs(demo_function());'
    ```


Run Omnigres in a Docker container. Make sure to mount the local directory as a volume on the correct path.
```shell
docker run --name omnigres \
           -e POSTGRES_PASSWORD=omnigres \
           -e POSTGRES_USER=omnigres \
           -e POSTGRES_DB=omnigres \
           --mount source=omnigres,target=/var/lib/postgresql/data -v $(pwd)/python-files:/python-files \
           -p 127.0.0.1:5433:5432 --rm ghcr.io/omnigres/omnigres-slim:latest
```

Let's try it out!
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

Create `omni_httpd` extension to be able to handle HTTP requests.
```postgresql
create extension if not exists omni_httpd cascade;
```

Add the following in `requirements.txt` file.
```
omni_http[Flask]
```

Create HTTP listeners for our Flask application. For example, let's add a HTTP listener for port 5000.
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

Let's say you have a table called `employees`.
```postgresql
create table employees (
   id integer primary key generated always as identity,
   name text not null,
   department text not null,
   salary integer not null
);
```

Now you can update your Python files (in the mounted volume) to include Flask functionality. For example, you can define endpoints to fetch list of all employees, fetch a particular employee, as well as create a new employee record.
``` py hl_lines="39-43"
from omni_python import pg
from omni_http import omni_httpd
from omni_http.omni_httpd import flask
from flask import Flask, jsonify, make_response
import uuid

app = Flask('myapp')

def employees_to_json(employees):
    return json.dumps([dict(employee) for employee in employees])

@app.route('/employees', methods=['POST'])
def create_employee():
    from flask import make_response, request
    json_data = json.loads(request.data.decode('UTF-8'))

    employee_name = json_data.get('name')
    employee_department = json_data.get('department')
    employee_salary = json_data.get('salary')

    if not employee_name or not employee_department or not employee_salary:
        return "Missing required fields", 400

    employee = plpy.execute(plpy.prepare("insert into employees (name, department, salary) "
                                         "values ($1, $2, $3) returning *", ["text", "text", "int"]),
                                        [employee_name, employee_department, employee_salary])
    return employees_to_json(employee)

@app.route('/employees', methods=['GET'])
def get_employees():
    employees = plpy.execute(plpy.prepare("select * from employees"))
    return employees_to_json(employees)

@app.route('/employees/<int:employee_id>', methods=['GET'])
def get_employee(employee_id):
    employee = plpy.execute(plpy.prepare("select * from employees where id = $1", ["int"]), [employee_id])
    return employees_to_json(employee)

app_ = flask.Adapter(app)

@pg
def handle(req: omni_httpd.HTTPRequest) -> omni_httpd.HTTPOutcome:
    return app_(req)
```

!!! info "Flask integration with Omnigres"

    `flask.Adapter(app)` creates an instance of the flask.Adapter class, which is provided by the `omni_http` library. 
    This adapter allows you to integrate Flask with the `omni_http` framework. 
    The `app` object is your Flask application instance, and you pass it to `flask.Adapter()` to create an adapter
    that can handle HTTP requests using your Flask app.

    The `handle` function is the entry point for handling incoming HTTP requests. 
    It takes an `HTTPRequest` object as input and is expected to return an `HTTPOutcome`. 
    Inside the function, it forwards the `req` object to the `app_` object, which is a Flask 
    application wrapped in the ``flask.Adapter()`. This allows the Flask application to handle 
    the incoming HTTP request and generate a response. Finally, the response is returned 
    as `HTTPOutcome`.

!!! tip "Tip"

    Note: We use `plpy` for now, but we should use DB API compatible APIs and/or other 
    frameworks (such as SQLAlchemy) which will be available very soon.

Make sure to add port mapping for 5000 when running Omnigres via Docker.
```shell
docker run --name omnigres \
           -e POSTGRES_PASSWORD=omnigres \
           -e POSTGRES_USER=omnigres \
           -e POSTGRES_DB=omnigres \
           --mount source=omnigres,target=/var/lib/postgresql/data -v $(pwd)/python-files:/python-files \
           -p 127.0.0.1:5450:5432 -p 127.0.0.1:5000:5000 --rm ghcr.io/omnigres/omnigres-slim:latest
```

You can hit the endpoints defined in the Flask code above.

Fetch all employees:
```shell
$ curl http://localhost:5000/employees
[
    {
        "id": 1,
        "name": "Akshat",
        "department": "Engineering",
        "salary": 100000
    },
    {
        "id": 2,
        "name": "Mohit",
        "department": "Sales",
        "salary": 50000
    }
]
```

Create a new employee:
```shell
$ curl --request POST \
  --url http://localhost:5000/employees \
  --header 'Content-Type: application/json' \
  --data '{
  "name": "Daniel",
  "department": "Marketing",
  "salary": 70000
}'
[{"id": 3, "name": "Daniel", "department": "Marketing", "salary": 70000}]
```

Fetch a particular employee:
```shell
$ curl http://localhost:5000/employees/3
[{"id": 3, "name": "Daniel", "department": "Marketing", "salary": 70000}]
```
