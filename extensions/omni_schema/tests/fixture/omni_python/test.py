from omni_python import pg


@pg
def fun1(v: str) -> int:
    return len(v)


@pg
def fun2(v: str) -> str:
    return v[::-1]


@pg
def add(v1: str, v2: str) -> str:
    from arithmetics import arithmetics
    return arithmetics.calculate(arithmetics.OPERAND_ADD, v1, v2)