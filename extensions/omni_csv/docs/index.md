# CSV Toolkit

This extension provides direct CSV parsing and encoding capabilities without having to use `COPY`,
enabling inline CSV processing within SQL queries and programmatic workflows.

## Installation and Setup

```postgresql
-- Install the extension
create extension omni_csv;

-- All functions are available in the omni_csv schema
```

## Core Functions

### `omni_csv.csv_info(csv text)` - Inspect CSV Structure

Analyze the structure of CSV data by extracting column names from the header row:

```postgresql
select *
from
    omni_csv.csv_info(E'name,age,city\nJohn,25,NYC\nJane,30,LA');
```

**Returns:**

```
 column_name 
-------------
 name
 age
 city
```

This function is essential for working with dynamic or unknown CSV structures, allowing you to discover the schema
before parsing the data.

**Use cases:**

- Validating CSV uploads before processing
- Building dynamic queries based on CSV structure
- API endpoints that accept arbitrary CSV formats

### `omni_csv.parse(csv text)` - Parse CSV Data

Transform CSV text into queryable records:

```postgresql
select *
from
    omni_csv.parse(E'name,age,city\nJohn,25,NYC\nJane,30,LA')
        as t(name text, age text, city text);
```

**Returns:**

```
 name | age | city 
------+-----+------
 John |  25 | NYC
 Jane |  30 | LA
```

**Key Requirements:**

- Must specify column structure using `as t(column_name text, ...)`
- The number of columns should match

### `omni_csv.csv_agg(record)` - Generate CSV from records

Convert query results back into CSV format:

```postgresql
select
    omni_csv.csv_agg(t)
from
    (select
         name,
         age,
         city
     from
         employees
     where
         department = 'Engineering'
     order by name) t;
```

**Returns:** A complete CSV string with headers and properly escaped values.

!!! warning "Limitations & caveats"

    Currently, on an empty set, this aggregate won't encode headers either, returning an empty string.

## Limitations and Considerations

- **Memory Usage**: Large CSV strings are processed entirely in memory
- **Type Declaration**: Column types must be explicitly specified in `as t(...)` clause
- **No Streaming**: Not suitable for extremely large datasets that exceed memory limits
- **Schema Rigidity**: Once declared, the column structure cannot be changed within the same query
- **Performance**: For repeated access to the same CSV, consider materializing the parsed data

## Comparison with Alternatives

| Method         | Use Case                        | Pros                                | Cons                        |
|----------------|---------------------------------|-------------------------------------|-----------------------------|
| `omni_csv`     | Programmatic, inline processing | No temp files, full SQL integration | Memory limited              |
| `COPY`         | Bulk data loading               | Very fast, streaming                | Requires file system access |
| External tools | Very large datasets             | Can handle any size                 | Additional dependencies     |

The `omni_csv` extension excels in scenarios where CSV data needs to be processed as part of complex SQL workflows, API
endpoints, or data transformation pipelines without the overhead of temporary files or external processes.
