CREATE FUNCTION switch_to_new()
    RETURNS bool
    AS 'MODULE_PATHNAME', 'switch_to_new'
    LANGUAGE C;

CREATE FUNCTION switch_back_to_old()
    RETURNS bool
    AS 'MODULE_PATHNAME', 'switch_back_to_old'
    LANGUAGE C;

CREATE FUNCTION switch_back_to_old_with_return()
    RETURNS bool
    AS 'MODULE_PATHNAME', 'switch_back_to_old_with_return'
    LANGUAGE C;

CREATE FUNCTION switch_when_finalizing()
    RETURNS bool
    AS 'MODULE_PATHNAME', 'switch_when_finalizing'
    LANGUAGE C;