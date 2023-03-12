SELECT omni_web.parse_query_string(NULL) IS NULL AS is_true;
SELECT omni_web.parse_query_string('');
SELECT omni_web.parse_query_string('a');
SELECT omni_web.parse_query_string('a=x&b=1');
SELECT omni_web.parse_query_string('a=x&b=1');
SELECT omni_web.parse_query_string('a&b=1');
SELECT omni_web.parse_query_string('a=%20&b=1');
SELECT omni_web.parse_query_string('a=%20&b=1+3');
