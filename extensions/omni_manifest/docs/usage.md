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
status (`status`, typed `requirement_status`, an enum of `installed`, `missing`, `updated`.

For each effective requirement in the install plan, it'll attempt to install a given version. If one is not available,
it'll report it as `missing`, otherwise, if a different version is installed it'll try to update and report it `updated`
if successful. Otherwise, it'll try to install it and return a value of `installed`.

!!! tip  "Install transactionally"

    It is highly recommended to perform installations transactionally to be able to check
    reports for missing dependencies.