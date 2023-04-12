<!-- @formatter:off -->
# Sum Types

[Sum types](https://en.wikipedia.org/wiki/Tagged_union) (also known as tagged unions, or enums) is a type that allows to hold a value that could take on several different types that are known ahead of time.

In Postgres context, this allows one to return values of different types in a single query column, or store different values in a column where maintaining separate columns or tables is excessive.

## Defining a sum type

One can define it using `omni_types.sum_type` function, passing the intended type name and the list of variant types.

Below, let's create a unified [geometric type](https://www.postgresql.org/docs/current/datatype-geometric.html)[^geom_type]:

[^geom_type]: PostGIS defines it is own [`geometry` type](https://postgis.net/docs/manual-3.3/using_postgis_dbmanagement.html#PostGIS_Geometry). Our definition is used to showcase a generalized approach.

```postgresql
omni_types=# select omni_types.sum_type('geom', 'point', 'line', 'lseg', 'box', 'path', 'polygon', 'circle');
sum_type
----------
 geom
(1 row)
```

We can now see it's been created:

```postgresql
omni_types=# \dT geom
list of data types
 schema | name | Description
--------+------+-------------
 public | geom |
(1 row)
         
omni_types=# table omni_types.sum_types;
oid  |                 variants
-------+-------------------------------------------
 16397 | {point,line,lseg,box,path,polygon,circle}
(1 row) 
```

## Textual representation

Sum type can be initialized using textual representatin, with the variant name used to
indicate the type:

```postgresql
omni_types=# select 'point(10,10)'::geom;
geom
----------------
 point((10,10))
(1 row)
```

By the virtue of seeing the above output, we know that it also converts back to a textual representation
using the underlying variant's representation.

## Conversion and casting

Sum types can be casted from and to their variants.

```postgresql
omni_types=# select '<(10,10),10>'::circle::geom;
geom
----------------------
 circle(<(10,10),10>)
(1 row)
             
omni_types=# select '<(10,10),10>'::circle::geom::circle;
circle
--------------
 <(10,10),10>
(1 row)
```

They can also be converted using functions following the pattern of
`<type>_from_<type>`:

```postgresql
omni_types=# select geom_from_point('10,10');
geom_from_point
-----------------
 point((10,10))
(1 row)
     
omni_types=# select point_from_geom(geom_from_point('10,10'));
point_from_geom
-----------------
 (10,10)
(1 row)
```

If one attempts to cast or convert to a wrong variant, `null` will be returned:

```postgresql
omni_types=# select '<(10,10),10>'::circle::geom::point;
point
-------
 null
(1 row)
omni_types=# select point_from_geom('<(10,10),10>'::circle::geom);
point_from_geom
-----------------
 null
(1 row)
```

??? warning "Caveat: casting domains"

    Due to the way [domains](https://www.postgresql.org/docs/current/sql-createdomain.html) work, casting
    is impossible as they are rather thin layers over their base types.

    That being said, the functions described above can be used to accomplish the same:

    ```postgresql
    omni_types=# create domain my_point as point;
    CREATE DOMAIN

    omni_types=# select omni_types.sum_type('my_geom','my_point');
    sum_type
    ----------
     my_geom
    (1 row)

    omni_types=# select my_geom_from_my_point('1,1');
     my_geom_from_my_point
    -----------------------
     my_point((1,1))
    (1 row)
    
    omni_types=# select my_point_from_my_geom(my_geom_from_my_point('1,1'));
     my_point_from_my_geom
    -----------------------
     (1,1)
    (1 row)
    ```

## Retrieving the variant type

One can determine the type of the variant to advise further processing:

```postgresql
omni_types=# select omni_types.variant('point(10,10)'::geom);
variant
---------
 point
(1 row)
```

## Changing the variant type

Sometimes it may be desirable to change the definition of a sum type. In most cases, it would
be prudent to define a new type and migrate to it. However, this may be undesirable to due
involved complexity. Luckily, under certain constraints, variant types __can__ be changed:

* Only __adding__ new variants is permitted
* For fixed-size sum types (those _not_ containing variable-sized variants), the size of the new variant __may not be larger__
  than that of the largest existing variant. [^fixed-size-alteration]

```postgresql
select omni_types.add_variant('geom', 'my_box');
```

[^fixed-size-alteration]: This is done so that Postgres would not try to read existing values using an updated, larger size, which
is erroneous.