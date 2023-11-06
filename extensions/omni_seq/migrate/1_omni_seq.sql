--## set sizes = {"int16": 2, "int32": 4, "int64": 8}
--## set typenames = {"int16": "int2", "int32": "int4", "int64": "int8"}
--## for prefix_type in ["int16","int32","int64"]
--## for value_type in  ["int16","int32","int64"]
--## set prefix_size = at(sizes, prefix_type)
--## set value_size = at(sizes, value_type)
--## if prefix_type == value_type
--## set name = "id_" + prefix_type
--## else
--## set name = "id_" + prefix_type + "_" + value_type
--## endif
--## set alignments = ["invalid", "invalid", "int2", "invalid", "int4", "invalid", "invalid", "invalid", "double"]
--## if prefix_size >= value_size
--## set alignment = at(alignments, prefix_size)
--## else
--## set alignment = at(alignments, value_size)
--## endif
--## set len = at(sizes, prefix_type) + at(sizes, value_type)
create type "/*{{ name }}*/";

create function "/*{{ name }}*/_in"(cstring) returns "/*{{ name }}*/"
as
'MODULE_PATHNAME' language c immutable
                             strict;

create function "/*{{ name }}*/_out"("/*{{ name }}*/") returns cstring
as
'MODULE_PATHNAME' language c immutable
                             strict;

create function "/*{{ name }}*/_send"("/*{{ name }}*/") returns bytea
as
'MODULE_PATHNAME' language c immutable
                             strict;

create function "/*{{ name }}*/_recv"(internal) returns "/*{{ name }}*/"
as
'MODULE_PATHNAME' language c immutable
                             strict;

create type "/*{{ name }}*/"
(
    input = "/*{{ name }}*/_in",
    output = "/*{{ name }}*/_out",
    internallength = /*{{ len  }}*/,
    alignment = /*{{ alignment }}*/,
    send = "/*{{ name }}*/_send",
    receive = "/*{{ name }}*/_recv"
);

create function "/*{{ name }}*/_eq"("/*{{ name }}*/", "/*{{ name }}*/") returns boolean as
'MODULE_PATHNAME' language c immutable
                             strict;
create function "/*{{ name }}*/_neq"("/*{{ name }}*/", "/*{{ name }}*/") returns boolean as
'MODULE_PATHNAME' language c immutable
                             strict;
create function "/*{{ name }}*/_leq"("/*{{ name }}*/", "/*{{ name }}*/") returns boolean as
'MODULE_PATHNAME' language c immutable
                             strict;
create function "/*{{ name }}*/_lt"("/*{{ name }}*/", "/*{{ name }}*/") returns boolean as
'MODULE_PATHNAME' language c immutable
                             strict;
create function "/*{{ name }}*/_geq"("/*{{ name }}*/", "/*{{ name }}*/") returns boolean as
'MODULE_PATHNAME' language c immutable
                             strict;
create function "/*{{ name }}*/_gt"("/*{{ name }}*/", "/*{{ name }}*/") returns boolean as
'MODULE_PATHNAME' language c immutable
                             strict;
create function "/*{{ name }}*/_cmp"("/*{{ name }}*/", "/*{{ name }}*/") returns int as
'MODULE_PATHNAME' language c immutable
                             strict;

create function "/*{{ name }}*/_nextval"("/*{{ at(typenames, prefix_type) }}*/", "regclass") returns "/*{{ name }}*/" as
'MODULE_PATHNAME' language c;

create function "/*{{ name }}*/_make"("/*{{ at(typenames, prefix_type) }}*/",
                                      "/*{{ at(typenames, value_type) }}*/") returns "/*{{ name }}*/" as
'MODULE_PATHNAME' language c;

create operator = (
    procedure = "/*{{ name }}*/_eq",
    leftarg = "/*{{ name }}*/",
    rightarg = "/*{{ name }}*/",
    commutator = =,
    negator = <>,
    restrict = eqsel,
    join = eqjoinsel,
    merges,
    hashes
    );

create operator <> (
    procedure = "/*{{ name }}*/_neq",
    leftarg = "/*{{ name }}*/",
    rightarg = "/*{{ name }}*/",
    commutator = <>,
    negator = =,
    restrict = neqsel,
    join = neqjoinsel
    );

create operator < (
    procedure = "/*{{ name }}*/_lt",
    leftarg = "/*{{ name }}*/",
    rightarg = "/*{{ name }}*/",
    commutator = >,
    negator = >=,
    restrict = scalarltsel,
    join = scalarltjoinsel
    );

create operator > (
    procedure = "/*{{ name }}*/_gt",
    leftarg = "/*{{ name }}*/",
    rightarg = "/*{{ name }}*/",
    commutator = <,
    negator = <=,
    restrict = scalargtsel,
    join = scalargtjoinsel
    );

create operator <= (
    procedure = "/*{{ name }}*/_leq",
    leftarg = "/*{{ name }}*/",
    rightarg = "/*{{ name }}*/",
    commutator = >=,
    negator = >,
    restrict = scalarltsel,
    join = scalarltjoinsel
    );

create operator >= (
    procedure = "/*{{ name }}*/_geq",
    leftarg = "/*{{ name }}*/",
    rightarg = "/*{{ name }}*/",
    commutator = <=,
    negator = <,
    restrict = scalargtsel,
    join = scalargtjoinsel
    );

create operator class "/*{{ name }}*/_ops"
    default for type "/*{{ name }}*/"
    using btree
    as
    operator 1 <,
    operator 2 <=,
    operator 3 =,
    operator 4 >= ,
    operator 5 > ,
    function 1 "/*{{ name }}*/_cmp"("/*{{ name }}*/", "/*{{ name }}*/");

--## endfor
--## endfor

create function system_identifier() returns bigint
as
'MODULE_PATHNAME' language c immutable;