-- params type
select array['key', 'value']::omni_web.params;
-- Invalid params should fail:
select array['key']::omni_web.params;

\pset null '<null>'
select omni_web.param_get(omni_web.parse_query_string('a=1&a=2'), 'a');
select omni_web.param_get(omni_web.parse_query_string('a&a=2'), 'a');
select omni_web.param_get_all(omni_web.parse_query_string('a&a=2'), 'a');
