---
title: Regression Testing
---

# Intro to pg_yregress

This tool takes original inspiration from `pg_regress` but addresses some
of the issues it has, such as:

* Unstructured tests [^psql-capture]
* Single Postgres instance operation [^single-instance]
* Lack of query/test re-use
* Binary encoding testing
* Per-query timeouts

[^psql-capture]: `pg_regress` relies on plain text `psql` session capture
and its comparison with the "baseline"
[^single-instance]: This prevents us from testing more complicated scenarios
such as replication or database links.

`pg_yregress` __core idea__ is to express test cases and setups using YAML
files for structuring and comparison. YAML sometimes carries some bad
reputation due to being overused in places where it doesn't fit well, as well
as somewhat surprising implicit tag resolution scheme results. 

However, the authors believe that YAML is actually a pretty good fit for the
use of grouping queries and expected results. Some features, like anchors
and references, are very helpful in succinct representation of tests.

!!! warning "pg_yregress is in its early days"

    Not all intended functionality has been implemented, and the one
    that has been, may suffer from critical bugs. The user experience is rather crude at the moment.

    None of this is impossible to overcome. Please consider
    [contributing](https://github.com/omnigres/omnigres/pulls).