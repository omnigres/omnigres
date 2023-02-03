CREATE TYPE statement;

CREATE FUNCTION statement_in(cstring) RETURNS statement
  AS 'MODULE_PATHNAME', 'statement_in'
  LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION statement_out(statement) RETURNS cstring
  AS 'MODULE_PATHNAME', 'statement_out'
  LANGUAGE C STRICT IMMUTABLE;

CREATE TYPE statement (
    INPUT = statement_in,
    OUTPUT = statement_out,
    LIKE = text
);

CREATE FUNCTION add_cte(statement, name text, cte statement,
                        recursive bool DEFAULT false, prepend bool DEFAULT false) RETURNS statement
  AS 'MODULE_PATHNAME', 'add_cte'
  LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION is_parameterized(statement) RETURNS bool
  AS 'MODULE_PATHNAME', 'is_parameterized'
  LANGUAGE C STRICT IMMUTABLE;