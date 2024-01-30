# Usage

omni_manifest allows to install and upgrade extensions as a set. Instead of doing this with `create extension`
and `alter extension` manually, especially when it comes to dependencies, this extension will take manifests (similar to
lock files in other systems) and provision them.

This extension is implemented in PL/pgSQL to simplify its provisioning (for example in environments where only trusted
languages are allowed.)

## Types

### Requirement

Requirement is a composite type (`omni_manifest.requirement`) that describes a `name` & `version` pair, both of
type `text`.

It can be converted to and from the text format of `name=version` through casting and the functions used in cast. Also,
an array (`requirement[]`)
can be converted to and from the text format of `name1=version1,name2=version2'.

Both the type and the array can be similarly converted to and from the JSON representation of
`"name": "version"`.

#### Examples

##### From

```postgresql
select ('myext=1.0'::text::omni_manifest.requirement).*
-- or
select ('{"myext": "1.0"}'::json::omni_manifest.requirement).*
```

```
 name  | version 
-------+---------
 myext | 1.0
(1 row)
```

```postgresql
select *
from
    unnest('myext=1.0,myotherext=2.0'::text::omni_manifest.requirement[])
-- or
select *
from
    unnest('{"myext": "1.0", "myotherext": "2.0"}'::json::omni_manifest.requirement[])
```

```
    name    | version 
------------+---------
 myext      | 1.0
 myotherext | 2.0
(2 rows)
```

##### To

```postgresql
select row ('myext','1.0')::omni_manifest.requirement::text
-- alternatively,
select row ('myext','1.0')::omni_manifest.requirement::json
```

```
    row    
-----------
 myext=1.0
(1 row)

# -- alternatively,

        row        
-------------------
 {"myext" : "1.0"}
(1 row)
```

```postgresql
select array [row ('myext','1.0'),row ('myotherext','2.0')]::omni_manifest.requirement[]::text
-- alternatively,
select array [row ('myext','1.0'),row ('myotherext','2.0')]::omni_manifest.requirement[]::json
```

```
          array           
--------------------------
 myext=1.0,myotherext=2.0
(1 row)

# -- alternatively,

omni_manifest=# select array[row('myext','1.0'),row('myotherext','2.0')]::omni_manifest.requirement[]::json;
                   array                   
-------------------------------------------
 { "myext" : "1.0", "myotherext" : "2.0" }
(1 row)
```

### Artifact

Artifact is a composite type (`omni_manifest.artifact`) that consists of a requirement (`self`,
typed `omni_manifest.requirement`) followed by its versioned requirements (`requirements`
of `omni_manifest.requirement[]`).

It can be constructed with the `omni_manifest.artifact` function:

```postgresql
 select (omni_manifest.artifact('ext=3.0'::text, 'myext=1.0,myotherext=2.0'::text)).*
```

```
   self    |            requirements            
-----------+------------------------------------
 (ext,3.0) | {"(myext,1.0)","(myotherext,2.0)"}
(1 row)
```

Besides that, artifacts can be constructed from and deconstructed to both text and JSON forms.

The text form of a single artifact is the text representation of a requirement optionally followed by `#` (the hash
sign) with a text representation of a list of requirements:

```
omni_manifest=1.0
omni_httpd=1.0#omni_http=1.0
```

The text form of multiple artifacts is a semicolon-separated list:

```
omni_manifest=1.0;omni_httpd=1.0#omni_http=1.0
```

!!! Tip

    For convenience, newline is also allowed instead of the semicolon:

    ```
    omni_manifest=1.0
    omni_httpd=1.0#omni_http=1.0
    ```

JSON representation is a JSON list of objects with a `target` property containing the target requirement,
and `requirements`
property containing all the requirements:

```json
[
  {
    "target": {
      "omni_manifest": "1.0"
    },
    "requirements": {}
  },
  {
    "target": {
      "omni_httpd": "1.0"
    },
    "requirements": {
      "omni_http": "1.0"
    }
  }
]
```

#### Examples

##### From

```postgresql
select *
from
    unnest('omni_manifest=1.0;omni_httpd=1.0#omni_http=1.0'::text::omni_manifest.artifact[]);
-- alternatively,
select *
from
    unnest('[{"target" : {"omni_manifest" : "1.0"}, "requirements" : {}}, {"target" : {"omni_httpd" : "1.0"}, "requirements" : { "omni_http" : "1.0" }}]'::json::omni_manifest.artifact[]);
```

```
        self         |    requirements     
---------------------+---------------------
 (omni_manifest,1.0) | 
 (omni_httpd,1.0)    | {"(omni_http,1.0)"}
(2 rows)
```

##### To

```postgresql
select
    '[{"target" : {"omni_manifest" : "1.0"}, "requirements" : {}}, {"target" : {"omni_httpd" : "1.0"}, "requirements" : { "omni_http" : "1.0" }}]'::json::omni_manifest.artifact[]::text;
```

```
omni_manifest=1.0;omni_httpd=1.0#omni_http=1.0
```

## Install Plan

From an array of artifacts, `omni_manifest.install_plan` can be used to show the order in which all extensions will be
installed to satisfy the requirements.

```postgresql
select *
from
    unnest(omni_manifest.install_plan(
            array [omni_manifest.artifact('ext=3.0'::text,
                                          'myext=1.0,myotherext=2.0'::text),
                omni_manifest.artifact('ext2=3.0'::text, 'myext=1.0'::text) ])) with ordinality t(name, version, position)
```

```
    name    | version | position 
------------+---------+----------
 myext      | 1.0     |        1
 myotherext | 2.0     |        2
 ext        | 3.0     |        3
 ext2       | 3.0     |        4
(4 rows)
```

!!! tip

    This step is not necessary for installation. It is primarily used to
    preview the steps that the install step will take.

## Install

Similar to `install_plan`, `omni_manifest.install` takes an array of artifacts and attempts to install them. It returns
a set of `omni_manifest.install_report` composite type values. Each row contains a requirement (`requirement`) and
status (`status`, typed `requirement_status`, an enum of `installed`, `missing`, `updated` or `kept`.

For each effective requirement in the install plan, it'll attempt to install a given version. If one is not available,
it'll report it as `missing`, otherwise, if a different version is installed it'll try to update and report it `updated`
if successful. Otherwise, it'll try to install it and return a value of `installed`, unless such version is already
installed, at which point it'll return `kept`.

!!! tip  "Install transactionally"

    It is highly recommended to perform installations transactionally to be able to check
    reports for missing dependencies.

## Updating extensions

To update extensions using `omni_manifest.install` please ensure the update scripts for the extensions are saved to the
Postgres extension scripts directory.

!!! tip "Where is this directory?"

    To find it, run the following command:

    ```shell
    echo $(pg_config --sharedir)/extension
    ```

Currently, all Omnigres extensions are versioned using git commit SHA from within Omnigres' repo, and extension update
is only possible from an older commit version to a newer one.

```postgresql
-- check available version of omni_python
select * from pg_available_extension_versions where name = 'omni_python';
    name     | version | installed | superuser | trusted | relocatable |   schema    |   requires   | comment 
-------------+---------+-----------+-----------+---------+-------------+-------------+--------------+---------
 omni_python | 2c7e737 | f         | t         | f       | f           | omni_python | {plpython3u} | 
(1 row) 

-- create omni_python extension with version '2c7e737'
select *                                
from
    omni_manifest.install(
            'omni_python=2c7e737#plpython3u=*'::text::omni_manifest.artifact[]
    );
      requirement      |  status   
-----------------------+-----------
 (plpython3u,*)        | installed
 (omni_python,2c7e737) | installed
(2 rows)
```

A public AWS S3 endpoint is available to download extension update scripts (old to new versions) for Omnigres-provided
extensions:

```bash
# download all the extension update scripts to a local directory
mkdir ext_updates && aws --no-sign-request s3 sync s3://omnigres-extensions/<postgres-major-version> ext_updates
```

To update `omni_python` to a later version for eg. `e135aa7` copy the following `omni_python` sql and control files
from `ext_updates` created above to postgres extension script directory
```bash
cp omni_python--2c7e737--e135aa7.sql omni_python--e135aa7.control <postgres-extension-script-directory>
```

```postgresql
-- recheck the available omni_python versions
select * from pg_available_extension_versions where name = 'omni_python';
    name     | version | installed | superuser | trusted | relocatable |   schema    |   requires   | comment 
-------------+---------+-----------+-----------+---------+-------------+-------------+--------------+---------
 omni_python | 2c7e737 | t         | t         | f       | f           | omni_python | {plpython3u} | 
 omni_python | e135aa7 | f         | t         | f       | f           | omni_python | {plpython3u} | 
(2 rows)

-- update omni_python to version 'e135aa7' and plpythonu3u version unchanged using '*'
select *                                
from
    omni_manifest.install(
            'omni_python=e135aa7#plpython3u=*'::text::omni_manifest.artifact[]
    );
      requirement      | status  
-----------------------+---------
 (plpython3u,*)        | kept
 (omni_python,e135aa7) | updated
(2 rows)

-- verify omni_python is updated
select * from pg_extension where extname = 'omni_python';
  oid  |   extname   | extowner | extnamespace | extrelocatable | extversion | extconfig | extcondition 
-------+-------------+----------+--------------+----------------+------------+-----------+--------------
 16484 | omni_python |       10 |        16483 | f              | e135aa7    |           | 
(1 row)
```

!!! tip "Artifact format of extensions"

    The artifact format of extensions can be copied from files named `artifacts-<version>`.txt in `ext_updates` created above