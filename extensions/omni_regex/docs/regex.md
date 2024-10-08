# Regex

This extensions provides [PCRE2](https://github.com/PCRE2Project/pcre2)-based regular expression functionality.
PCRE2 is feature-rich (for example, it provides named capture groups) and performant.

# `regex` type

This extension introduces a `regex` type which is effectively wrapping
a regular expression and defines operators and functions over it.

## Matching

Regular expressions can be matched against strings using `~` (with `!~` being
non-matching) or `=~` operators:

```sql
select 'foo' ~ regex 'fo+';
--#> t
select 'bar' !~ regex 'foo';
--#> t
-- they also work the other way around:
select regex 'fo+' ~ 'foo';
--#> t
select regex 'foo' !~ 'bar';
--#> t
-- Same as `~`
select 'foo' =~ regex 'fo+';
--#> t
```

### Extracting matches

`regex_match(text, regex)` will return `text[]` with all captured groups. If there are no
groups, will return an array with a single element:

```sql
select regex_match('ABC123', 'A.*');
--#> {ABC123}
select regex_match('ABC123', '([A-Z]*)(\d+)');
--#> {ABC,123}
```

To extract **multiple** matches, use `regex_matches(text, regex)`. It returns a `setof text[]`:

```sql
select regex_matches('foo1bar', '(fo+|bar)(\d?)');
--#> {foo,1}
--#> {bar,""}
```

## Named catpure groups

PCRE2 supports named capture groups (`(?<name>REGEX)`) and you can use `regex_named_groups(regex)`
to extract these. It returns a `table (name cstring, index int)` (where `index` is a 1-based index
of groups)

```sql
select index
from regex_named_groups('(fo+|bar)(?<num>\d?)')
where name = 'num';
--#> 2
```

# Credits

The extension is a fork of [pgpcre](https://github.com/petere/pgpcre) by
Peter Eisentraut with [unmerged](https://github.com/petere/pgpcre/pull/9) contributions from Christoph Berg (pcre2
support), modified to support named capture groups, parallelization and PCRE2.

The original code is licensed under the terms of The PostgreSQL License reproduced below.

??? tip "License"

    Copyright © 2013, Peter Eisentraut <peter@eisentraut.org>

    (The PostgreSQL License)
    
    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose, without fee, and without a written
    agreement is hereby granted, provided that the above copyright notice
    and this paragraph and the following two paragraphs appear in all
    copies.
    
    IN NO EVENT SHALL THE AUTHOR(S) OR ANY CONTRIBUTOR(S) BE LIABLE TO ANY
    PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
    DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS
    SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR(S) OR
    CONTRIBUTOR(S) HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    
    THE AUTHOR(S) AND CONTRIBUTOR(S) SPECIFICALLY DISCLAIM ANY WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE
    PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE AUTHOR(S) AND
    CONTRIBUTOR(S) HAVE NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT,
    UPDATES, ENHANCEMENTS, OR MODIFICATIONS.