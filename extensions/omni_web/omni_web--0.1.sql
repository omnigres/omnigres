CREATE FUNCTION parse_query_string(query_string text) RETURNS text[]
   STRICT IMMUTABLE
   AS 'MODULE_PATHNAME', 'parse_query_string' LANGUAGE C;