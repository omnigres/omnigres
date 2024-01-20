create function artifacts_to_text(artifacts artifact[]) returns text
    language sql
    immutable as
$$
with
    arts as (select
                 a::text
             from
                 unnest(artifacts) a)
select
    string_agg(a, ';')
from
    arts
$$;