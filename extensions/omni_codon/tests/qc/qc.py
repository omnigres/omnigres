from omni_python import pg

from hypothesis import given, strategies as st, settings, Verbosity
import math
import numbers


class sized_integer:
    def __init__(self, size, i):
        self.size = size
        self.value = i

    def codon_name(self):
        return f"Int[{self.size}]" if self.size != 64 else "int"

    def postgres_name(self):
        bytes = int(self.size / 8)
        return f"int{bytes}"

    def __str__(self):
        return f"int{self.size}({self.value})"

    def __repr__(self):
        return str(self)


def ints(width):
    return st.integers(min_value=-2 ** (width - 1), max_value=2 ** (width - 1) - 1).map(
        lambda x: sized_integer(width, x))


@given(ints(16))
def test_valid_int16(i):
    assert i.value >= -2 ** (16 - 1) and i.value <= 2 ** (16 - 1) - 1, "must be within range"
    assert i.size == 16, "must have correct size"


@given(ints(32))
def test_valid_int32(i):
    assert i.value >= -2 ** (32 - 1) and i.value <= 2 ** (32 - 1) - 1, "must be within range"
    assert i.size == 32, "must have correct size"


@given(ints(64))
def test_valid_int64(i):
    assert i.value >= -2 ** (64 - 1) and i.value <= 2 ** (64 - 1) - 1, "must be within range"
    assert i.size == 64, "must have correct size"


@pg
def qc_valid_int16() -> bool:
    test_valid_int16()
    return True


@pg
def qc_valid_int32() -> bool:
    test_valid_int32()
    return True


@pg
def qc_valid_int64() -> bool:
    test_valid_int64()
    return True


import plpy


class text_string:
    def __init__(self, s):
        self.value = s

    def postgres_name(self):
        return "text"

    def codon_name(self):
        return "str"

    def __str__(self):
        return f"str({self.value})"

    def __repr__(self):
        return str(self)


def strings():
    return st.text(st.characters(codec='latin-1', exclude_characters=['\0'])).map(text_string)


class sized_float:
    def __init__(self, size, i):
        self.size = size
        self.value = i

    def codon_name(self):
        return "float32" if self.size == 32 else "float"

    def postgres_name(self):
        return "float4" if self.size == 32 else "float8"

    def __str__(self):
        return f"float{self.size}({self.value})"

    def __repr__(self):
        return str(self)


def floats(width):
    return st.floats(width=width).map(
        lambda x: sized_float(width, x))

class Rollback(Exception):
    pass


@given(st.lists(ints(16) | ints(32) | ints(64) | strings() | floats(32) | floats(64), min_size=1), st.data())
@settings(max_examples=2000, verbosity=Verbosity.verbose, deadline=None)
def test_codon_identity(vs, data):
    return_index = data.draw(st.integers(min_value=0, max_value=len(vs) - 1))
    v = vs[return_index]
    try:
        with plpy.subtransaction():
            # plpy.notice(f"{v}")
            s = plpy.prepare(
                f"create or replace function codon_identity({",".join([v.postgres_name() for v in vs])}) returns {v.postgres_name()} language 'plcodon' as \n\
                 $$\n\
@pg\n\
def codon_identity({",".join(["i" + str(i) + ": " + v.codon_name() for i, v in enumerate(vs)])}) -> {v.codon_name()}:\n\
    return i{str(return_index)}\n\
$$")
            plpy.execute(s)
            s = plpy.prepare(f"select codon_identity({",".join(["$" + str(v) for v in range(1, len(vs) + 1)])})",
                             [v.postgres_name() for v in vs])
            val = plpy.execute(s, [v.value for v in vs])[0]['codon_identity']
            if val != vs[return_index].value:
                if isinstance(val, numbers.Number) and math.isnan(vs[return_index].value) and not math.isnan(val):
                    plpy.notice(f"identity({", ".join([str(v) for v in vs])},{return_index}) -> {v}")
                    plpy.notice(f"expected {vs[return_index].value} got {val}")
            assert (val == vs[return_index].value) or (
                    isinstance(val, numbers.Number) and math.isnan(vs[return_index].value) and math.isnan(
                val)), f"identity for {v} failed (expected {vs[return_index].value} at index {return_index}, got {val})"
            raise Rollback()

    except Rollback:
        pass
    except Exception as e:
        plpy.notice(f"error with {v} /{v.postgres_name()}->{v.codon_name()}/: {e}")
        raise Exception(f"error with {v} /{v.postgres_name()}->{v.codon_name()}/: {e}")



@pg
def qc_codon_identity() -> bool:
    test_codon_identity()
    return True


@given(ints(16) | ints(32) | ints(64) | strings() | floats(32) | floats(64))
@settings(max_examples=2000, verbosity=Verbosity.verbose, deadline=None)
def test_codon_identity_optional(v):
    try:
        with plpy.subtransaction():
            # plpy.notice(f"{v}")
            s = plpy.prepare(
                f"create or replace function codon_identity({v.postgres_name()}) returns {v.postgres_name()} language 'plcodon' as \n\
                 $$\n\
@pg\n\
def codon_identity(i: Optional[{v.codon_name()}]) -> Optional[{v.codon_name()}]:\n\
    return i\n\
$$")
            plpy.execute(s)
            s = plpy.prepare("select codon_identity($1)", [v.postgres_name()])
            val = plpy.execute(s, [v.value])[0]['codon_identity']
            plpy.notice(f"{v}")
            assert (val == v.value) or (isinstance(val, numbers.Number) and math.isnan(val) and math.isnan(
                v.value)), f"identity for {v} failed (expected {v.value}, got {val})"
            assert plpy.execute(s, [None])[0][
                       'codon_identity'] is None, f"identity for `null` ({v.postgres_name()} failed"
            raise Rollback()

    except Rollback:
        pass
    except Exception as e:
        plpy.notice(f"error with {v} /{v.postgres_name()}->{v.codon_name()}/: {e}")
        raise Exception(f"error with {v} /{v.postgres_name()}->{v.codon_name()}/: {e}")


@pg
def qc_codon_identity_optional() -> bool:
    test_codon_identity_optional()
    return True
