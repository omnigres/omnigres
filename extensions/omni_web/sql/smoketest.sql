select omni_web.parse_query_string(null) is null as is_true;
select omni_web.parse_query_string('');
select omni_web.parse_query_string('a');
select omni_web.parse_query_string('a=x&b=1');
select omni_web.parse_query_string('a=x&b=1');
select omni_web.parse_query_string('a&b=1');
select omni_web.parse_query_string('a=%20&b=1');
select omni_web.parse_query_string('a=%20&b=1+3');

-- Ensure it works with binaries that can be converted
-- to strings
select omni_web.parse_query_string(convert_to('a=x&b=1', 'UTF8'));

-- Ensure it fails with an arbitrary binary
select omni_web.parse_query_string(E'\x0000'::bytea);
