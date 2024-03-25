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

## Download omnigres extensions

Omnigres extensions can be downloaded from public domain `index.omnigres.com`.

### Index endpoint

To view the extensions available for a given platform combination call
the `<pg_major_version>/<build_type>/<os-arch>/index.json` endpoint.

`Release` builds are available for following platforms:

| os-arch       | pg_major_versions |
|---------------|-------------------|
| ubuntu-x86-64 | 13, 14, 15, 16    |
| macos-arm     | 16                |

The response consists of all available extension versions:

```shell
$ curl -s https://index.omnigres.com/16/Release/ubuntu-x86-64/index.json | jq .
{
  "os_arch": "ubuntu-x86-64",
  "build_type": "Release",
  "extensions": {
    "omni": {
    # version: commit SHA at time of release
      "0.1.0": "3fb44b2d70622b8599282f610b9939f560981d83"
    },
    "omni_httpd": {
      "0.1.0": "3fb44b2d70622b8599282f610b9939f560981d83"
    },
    "omni_manifest": {
      "0.1.0": "3fb44b2d70622b8599282f610b9939f560981d83"
    }
  },
  "pg_version": "16",
  "format_version": 1
}

```

The request fails with an error message if an unsupported platform combination is used

```shell
$ curl -s https://index.omnigres.com/12/Release/ubuntu-x86-64/index.json
pg_version should be one of the following: 13,14,15,16
```

A script is available to download and copy omnigres extension files(if it doesn't exist) to appropriate postgres directories at
<https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh>

It is recommended to always run latest version of script as follows:
```shell
# use no arguments for the script help output
$ curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh | bash -s
```

!!! tip "Where are the postgres directories for extension files?"

    To find it, run the following command:

    ```shell
    # .sql and .control files location
    echo $(pg_config --sharedir)/extension
    ```
    
    ```shell
    # .so files location
    echo $(pg_config --pkglibdir)
    ```

### Prerequisites

Omnigres extensions require `omni` loaded as shared preload library. It is also recommended to use
the `omni_manifest` to create and update extensions in postgres.

```shell
# download omni extension for it's shared object library
$ curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh | bash -s install omni "0.1.0"
'/tmp/omni--0.1.0--untarred/omni--0.1.0.sql' -> '/usr/share/postgresql/16/extension/omni--0.1.0.sql'
'/tmp/omni--0.1.0--untarred/omni--0.1.0.control' -> '/usr/share/postgresql/16/extension/omni--0.1.0.control'
'/tmp/omni--0.1.0--untarred/omni.control' -> '/usr/share/postgresql/16/extension/omni.control'
'/tmp/omni--0.1.0--untarred/lib/omni--0.1.0.so' -> '/usr/lib/postgresql/16/lib/omni--0.1.0.so'

The omni_manifest query to create the extension:
      select *
      from
          omni_manifest.install('omni=0.1.0'::text::omni_manifest.artifact[])

# download omni_manifest extension for creating other extensions
$ curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh | bash -s install omni_manifest "0.1.0"
'/tmp/omni_manifest--0.1.0--untarred/omni_manifest--0.1.0.sql' -> '/usr/share/postgresql/16/extension/omni_manifest--0.1.0.sql'
'/tmp/omni_manifest--0.1.0--untarred/omni_manifest--0.1.0.control' -> '/usr/share/postgresql/16/extension/omni_manifest--0.1.0.control'
'/tmp/omni_manifest--0.1.0--untarred/omni_manifest.control' -> '/usr/share/postgresql/16/extension/omni_manifest.control'
```

Add `omni` shared object library to `shared_preload_libraries` and increase `max_worker_processes` in `postgresql.conf`:

```text
# add omni shared object library
shared_preload_libraries = 'omni--0.1.0.so'

# some omnigres extensions use postgres background workers, set it to high enough value
max_worker_processes = 64
```

Restart the postgres server after making changes to `postgresql.conf`:

```shell
# postgres restart required after changing the above settings 
pg_ctl -D <PGDATA> restart
```

Create `omni_manifest` extension in postgres to create and update other extensions:

```postgresql
create extension omni_manifest version '0.1.0';
```

```
postgres=# \dx
                      List of installed extensions
     Name      | Version |    Schema     |         Description          
---------------+---------+---------------+------------------------------
 omni_manifest | 0.1.0   | omni_manifest | 
 plpgsql       | 1.0     | pg_catalog    | PL/pgSQL procedural language
(2 rows)
```

### Download and install an extension

To download an omnigres extension call
the `<pg_major_version>/<build_type>/<os-arch>/<ext-name>/<ext-version>`
endpoint. A compressed tarball containing extension files will be sent in response.
The list of extensions and its released versions is available at [`index.json`](#index-endpoint) endpoint.

!!! tip  "Downloading extension with its dependencies"

     `<pg_major_version>/<build_type>/<os-arch>/<ext-name>/<ext-version>?with-dependencies`
     endpoint will download an extension along with all it's dependency files.

     `with-dependencies` query parameter is optional, drop it only if extension files 
     without its dependencies files is required. It is highly recommended to use it
     because dependencies of an extension may change between versions.

#### Download extension files

```shell
# download omni_httpd 0.1.0 along with its dependencies
$ curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh | bash -s install omni_httpd "0.1.0"
'/tmp/omni_httpd--0.1.0--untarred/omni_http--0.1.0.sql' -> '/usr/share/postgresql/16/extension/omni_http--0.1.0.sql'
'/tmp/omni_httpd--0.1.0--untarred/omni_httpd--0.1.0.sql' -> '/usr/share/postgresql/16/extension/omni_httpd--0.1.0.sql'
'/tmp/omni_httpd--0.1.0--untarred/omni_types--0.1.0.sql' -> '/usr/share/postgresql/16/extension/omni_types--0.1.0.sql'
'/tmp/omni_httpd--0.1.0--untarred/omni_http--0.1.0.control' -> '/usr/share/postgresql/16/extension/omni_http--0.1.0.control'
'/tmp/omni_httpd--0.1.0--untarred/omni_http.control' -> '/usr/share/postgresql/16/extension/omni_http.control'
'/tmp/omni_httpd--0.1.0--untarred/omni_httpd--0.1.0.control' -> '/usr/share/postgresql/16/extension/omni_httpd--0.1.0.control'
'/tmp/omni_httpd--0.1.0--untarred/omni_httpd.control' -> '/usr/share/postgresql/16/extension/omni_httpd.control'
'/tmp/omni_httpd--0.1.0--untarred/omni_types--0.1.0.control' -> '/usr/share/postgresql/16/extension/omni_types--0.1.0.control'
'/tmp/omni_httpd--0.1.0--untarred/omni_types.control' -> '/usr/share/postgresql/16/extension/omni_types.control'
'/tmp/omni_httpd--0.1.0--untarred/lib/omni_httpd--0.1.0.so' -> '/usr/lib/postgresql/16/lib/omni_httpd--0.1.0.so'
'/tmp/omni_httpd--0.1.0--untarred/lib/omni_types--0.1.0.so' -> '/usr/lib/postgresql/16/lib/omni_types--0.1.0.so'

The omni_manifest query to create the extension:
      select *
      from
          omni_manifest.install('omni_httpd=0.1.0#omni_types=0.1.0,omni_http=0.1.0'::text::omni_manifest.artifact[])
```

#### Install extension

Use the `omni_manifest.install` query of the extension from the script output above.

```postgresql
select *
from
    omni_manifest.install('omni_httpd=0.1.0#omni_types=0.1.0,omni_http=0.1.0'::text::omni_manifest.artifact[]);
```

```
    requirement     |  status   
--------------------+-----------
 (omni_http,0.1.0)  | installed
 (omni_types,0.1.0) | installed
 (omni_httpd,0.1.0) | installed
(3 rows)
```

List of installed extensions:

```
postgres=# \dx
                      List of installed extensions
     Name      | Version |    Schema     |         Description          
---------------+---------+---------------+------------------------------
 omni_http     | 0.1.0   | omni_http     | 
 omni_httpd    | 0.1.0   | omni_httpd    | 
 omni_manifest | 0.1.0   | omni_manifest | 
 omni_types    | 0.1.0   | omni_types    | 
 plpgsql       | 1.0     | pg_catalog    | PL/pgSQL procedural language
(5 rows)

```

## Update omnigres extensions

To update extensions using `omni_manifest.install`
please download newer version of the extension.

```shell
# download omni_httpd 0.2.0 along with its dependencies for update
$ curl -s https://raw.githubusercontent.com/omnigres/omnigres/master/download-omnigres-extension.sh | bash -s install omni_httpd "0.2.0"
'/tmp/omni_httpd--0.2.0--untarred/omni_httpd--0.2.0.sql' -> '/usr/share/postgresql/16/extension/omni_httpd--0.2.0.sql'
'/tmp/omni_httpd--0.2.0--untarred/omni_httpd--0.1.0--0.2.0.sql' -> '/usr/share/postgresql/16/extension/omni_httpd--0.1.0--0.2.0.sql'
'/tmp/omni_httpd--0.2.0--untarred/omni_httpd--0.2.0.control' -> '/usr/share/postgresql/16/extension/omni_httpd--0.2.0.control'
'/tmp/omni_httpd--0.2.0--untarred/lib/omni_httpd--0.2.0.so' -> '/usr/lib/postgresql/16/lib/omni_httpd--0.2.0.so'

The omni_manifest query to create the extension:
      select *
      from
          omni_manifest.install('omni_httpd=0.2.0#omni_types=0.1.0,omni_http=0.1.0'::text::omni_manifest.artifact[])

```

After the download of newer version, verify it's available:

```postgresql
-- check the available omni_httpd versions
select *
from
    pg_available_extension_versions
where
    name = 'omni_httpd';
```

```
    name    | version | installed | superuser | trusted | relocatable |   schema   |        requires        | comment 
------------+---------+-----------+-----------+---------+-------------+------------+------------------------+---------
 omni_httpd | 0.1.0   | t         | t         | f       | f           | omni_httpd | {omni_types,omni_http} | 
 omni_httpd | 0.2.0   | f         | t         | f       | f           | omni_httpd | {omni_types,omni_http} | 
(2 rows)
```

Install the newer version of the extension using the `omni_manifest.install` query of the extension from the script output

```postgresql
select *
from
    omni_manifest.install('omni_httpd=0.2.0#omni_types=0.1.0,omni_http=0.1.0'::text::omni_manifest.artifact[])
```

```
    requirement     |  status   
--------------------+-----------
 (omni_http,0.1.0)  | kept
 (omni_types,0.1.0) | kept
 (omni_httpd,0.2.0) | updated
(3 rows)
```

Verify omni_httpd is updated:

```
postgres=# \dx
                      List of installed extensions
     Name      | Version |    Schema     |         Description          
---------------+---------+---------------+------------------------------
 omni_http     | 0.1.0   | omni_http     | 
 omni_httpd    | 0.2.0   | omni_httpd    | 
 omni_manifest | 0.1.0   | omni_manifest | 
 omni_types    | 0.1.0   | omni_types    | 
 plpgsql       | 1.0     | pg_catalog    | PL/pgSQL procedural language
(5 rows)

```

!!! tip "Artifact format of extensions"

    The artifact format of an extension can be copied from script output