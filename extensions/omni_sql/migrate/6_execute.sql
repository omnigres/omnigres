create function execute_parameterized(stmt text, parameters jsonb, types regtype[]) returns setof record
    language c as
'MODULE_PATHNAME';

/*{% include "../src/execute.sql" %}*/