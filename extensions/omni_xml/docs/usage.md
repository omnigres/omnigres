# Usage

## XPath querying

One can query an XML document using XPath 1.0 queries using `omni_xml.xpath`
function. The function takes an XML document as `text` and an XPath query as 
`text`.

It returns a table with `path` (path to the node) and `value` (node value).

### Element

When searching for elements, it will return entire nodes serialized as XML.

```postgresql
select * from omni_xml.xpath('<node>value</node>', '/node')
```    

Results in:

```
 path  |       value        
-------+--------------------
 /node | <node>value</node>
(1 row)

```

### Text

When searching for textual data, one can use `text()` in their queries.

```postgresql
select * from omni_xml.xpath('<node>value</node>', '/node/text()')
```    

Results in:

```
  path  | value 
--------+-------
 /node/ | value
(1 row)
```

### Namespaces

It is possible to query using an explicit namespace identifier.

```postgresql
select * from omni_xml.xpath('<ns:node>value</ns:node>', '/ns:node')
```

Results in:

```
   path   |          value           
----------+--------------------------
 /ns:node | <ns:node>value</ns:node>
```

!!! warning "Proper namespace support is lacking"

    This is not ideal if the identifier used is unknown ahead of time,
    but sufficient if used for API response handling when it is typically the same.
